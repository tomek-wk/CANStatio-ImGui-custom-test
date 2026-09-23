# Specyfikacja eksperymentu CANStatio ImPlot Test

## 1. Cel

Projekt jest samodzielnym prototypem do sprawdzenia Dear ImGui + ImPlot jako potencjalnej warstwy wykresu dla CANstatio LogViewer.

Nie testujemy całej aplikacji LogViewer. Interesuje nas wykres oraz funkcje bezpośrednio związane z jego obsługą.

Główne pytania eksperymentu:

- czy ImPlot pozwala wygodnie osiągnąć model wielu serii z niezależnym Y i wspólnym X;
- ile funkcjonalności jest natywne, a ile trzeba dopisać;
- czy własne interakcje dają się pogodzić z natywnym pan/zoom ImPlot;
- jaka jest praktyczna responsywność przy danych zbliżonych do planowanego LogViewera;
- czy koszt kodu i utrzymania jest akceptowalny.

Priorytetem jest funkcjonalność i koszt implementacji. Wydajność jest obserwowana, ale nie definiujemy na starcie twardego progu FPS.

## 2. Zakres i rzeczy świadomie wykluczone

W zakresie:

- generowane dane w pamięci;
- wiele serii na jednym wykresie;
- wspólna oś czasu X;
- niezależny Y każdej serii;
- aktywna seria;
- widoczność serii;
- własna semantyczna oś Y aktywnej serii;
- własny grid aktywnej serii;
- pan/zoom X;
- pan/zoom Y aktywnej serii;
- Fit X i Fit Y;
- wybór serii przez trafienie w linię;
- legenda natywna i własna;
- Values pod myszą;
- crosshair czasu;
- kursory A/B;
- markery czasu;
- ręczna obserwacja FPS i frame time;
- zestawy danych do testów funkcjonalnych i stress testu;
- automatyczne testy funkcyjne logiki C++;
- automatyczne testy integracyjne bieżącego GUI Dear ImGui / ImPlot.

Poza zakresem pierwszej wersji:

- parser plików;
- CSV/import danych;
- DataCore;
- format projektu LogViewera;
- irregular sampling;
- osobne timestampy każdej serii;
- NaN/gaps i specjalna semantyka przerw;
- sygnały step/digital;
- Overview/minimapa;
- LOD/downsampling;
- integracja z głównym CANstatio;
- zewnętrzna OS-level automatyzacja GUI, dopóki Dear ImGui Test Engine wiarygodnie pokrywa bieżące potrzeby.

## 3. Stack

- C++20
- Windows jako pierwsza platforma
- GCC / MinGW-w64 z MSYS2 UCRT64
- CMake + Ninja
- GLFW
- OpenGL 3.3 Core Profile
- Dear ImGui
- ImPlot
- Dear ImGui Test Engine dla integracyjnych testów GUI
- GoogleTest + CTest dla testów funkcyjnych C++

Dear ImGui, ImPlot, GLFW i Dear ImGui Test Engine są pobierane przez `FetchContent` z przypiętych referencji w `CMakeLists.txt`.

GoogleTest jest używany jako istniejący lokalny pakiet znaleziony przez `find_package(GTest REQUIRED)`; projekt nie pobiera go przez `FetchContent`.

Bazowe wersje eksperymentu:

- Dear ImGui `v1.92.9b`
- ImPlot `v1.0`
- GLFW `3.5.1`
- Dear ImGui Test Engine commit `508a8fc8dacac2f346d353fed31b9bc90ed29adc`

## 4. Główne okno i pomocniczy UI

Wykres ma wizualnie być aplikacją, a nie małym widgetem wewnątrz klasycznego okna ImGui.

- główna powierzchnia ImGui zajmuje cały client area GLFW;
- wykres automatycznie wypełnia dostępne miejsce;
- po resize dopasowuje się bez stałego aspect ratio;
- marginesy mają być małe;
- brak dockingu na tym etapie.

Pływające okna pomocnicze:

- `Test Controls`;
- `Values`;
- `Custom Legend`.

Okna są przesuwalne i można je ukryć/zamknąć. Zamknięcie nie niszczy stanu. `Test Controls` pozwala ponownie je pokazać.

`Test Controls` ma docelowo zawierać:

- wybór presetu danych;
- `Fit X`;
- `Fit Y`;
- `Reset`;
- wybór trybu wyróżnienia active: `Halo` / `Outline`;
- przełączniki widoczności Values, Custom Legend i Native Legend;
- przełącznik crosshair;
- metadane datasetu;
- FPS;
- frame time w ms;
- ewentualne inne przełączniki potrzebne do porównania zachowań testowych.

## 5. Model danych

W pierwszej wersji wszystkie serie mają wspólną, regularną oś czasu.

Dane datasetu:

- `N` — liczba próbek na serię;
- `dt` — stały krok czasu;
- czas próbki `i`: `x = i * dt`;
- każda seria przechowuje tylko `std::vector<float>` wartości Y;
- nie przechowujemy osobnej tablicy timestampów dla każdej serii.

Generator jest deterministyczny. Ten sam preset ma dawać te same dane przy każdym uruchomieniu.

Każda seria ma co najmniej:

- nazwę;
- kolor;
- `visible`;
- `dataMin` / `dataMax` dla całej serii;
- `viewMin` / `viewMax` opisujące aktualny stan Y;
- wartości Y.

## 6. Presety generatora

Bazowy zestaw presetów:

1. **Small / Sanity** — 3 serie × 1 000 próbek, `dt = 100 ms`.
2. **Reference** — 60 × 36 000, `dt = 100 ms`, dokładnie 1 godzina.
3. **Overlap** — 60 × 36 000, dużo przecinających i nakładających się przebiegów.
4. **Mixed Scale** — 60 × 36 000, bardzo różne zakresy, np. `0..8000`, `0..100`, `-1..1`, `100.001..100.009`.
5. **Spikes / Noise** — 60 × 36 000, szum, szybkie zmiany i izolowane piki.
6. **Long Time** — około 10 × 108 000, `dt = 100 ms`, 3 godziny.
7. **Stress Raw** — 60 × 360 000, `dt = 10 ms`, 1 godzina, 21.6 mln wartości.

`Stress Raw` służy do znalezienia granicy, nie jako podstawowy wymagany przypadek płynności.

Na początku renderujemy raw data bez LOD. LOD dodajemy dopiero, jeśli pomiar pokaże realną potrzebę.

## 7. Reset i zmiana datasetu

`Reset` oraz zmiana presetu datasetu prowadzą do tego samego stanu początkowego:

- X pokazuje pełny dataset `0..duration`;
- wszystkie serie są widoczne;
- brak aktywnej serii;
- każda seria ma niezależnie wykonane Fit Y z całego swojego datasetu;
- Fit Y dodaje około 5% marginesu z góry i z dołu;
- kursory A/B są ukryte;
- wszystkie markery są usunięte;
- licznik nowego markera wraca do `0`.

Dla serii stałej (`dataMin == dataMax`) należy utworzyć mały sztuczny zakres wokół wartości, aby uniknąć zakresu zerowego.

Usunięcie markera podczas normalnej pracy nie powoduje ponownego użycia jego numeru. Licznik wraca do zera tylko przy pełnym Reset.

## 8. Oś czasu X

X jest czasem względnym od początku datasetu. Początek to `00`, bez absolutnej daty/godziny.

Oś X:

- zawsze na dole;
- bez tytułu `Time`;
- etykiety pod pionowymi liniami grida;
- format adaptowany do aktualnego zoomu;
- po przekroczeniu godziny używa godzin.

Przykładowe formaty zależnie od skali:

- sekundy i minuty;
- `mm:ss`;
- `hh:mm:ss`;
- dokładniejszy format z częścią milisekundową po dużym zoomie.

Ticki X są generowane przez warstwę aplikacji na podstawie:

- `xMin/xMax`;
- szerokości wykresu w pikselach;
- zestawu „nice intervals”, np. 100 ms, 500 ms, 1 s, 5 s, 10 s, 30 s, 1 min, 5 min itd.

Nie dążymy do maksymalnego zagęszczenia etykiet. Priorytetem jest czytelny odstęp.

Główne pionowe linie grida mają dokładnie odpowiadać opisanym tickom czasu. Dopuszczalne są lżejsze linie minor bez etykiet.

Stan X jest własnością aplikacji (`xMin/xMax`) i ma pozostać zsynchronizowany z natywną nawigacją ImPlot.

- zwykły LMB drag — natywny pan X, jeśli nie przejęło go narzędzie;
- zwykłe kółko — natywny zoom X;
- `Fit X` — ustawia pełny czas datasetu;
- standardowy pan/zoom Y ImPlot jest wyłączony;
- box select nie jest częścią pierwszej wersji.

Nie dodajemy na tym etapie specjalnego clampa nawigacji X do granic datasetu. Kursory i markery są natomiast clampowane do datasetu.

## 9. Niezależny Y — kluczowy eksperyment

Każda seria utrzymuje własny stan:

- `viewMin`;
- `viewMax`.

Nie używamy wspólnego semantycznego Y dla wszystkich serii i nie tworzymy klasycznych wielu osi Y ImPlot.

Wewnętrzna fizyczna oś Y wykresu ImPlot pozostaje zablokowana na `0..1`.

Dla każdej serii surowa wartość Y jest mapowana do tej przestrzeni:

```text
normalized = (value - viewMin) / (viewMax - viewMin)
```

Wartości mogą po transformacji wyjść poniżej 0 lub powyżej 1 i zostać naturalnie przycięte przez plot area.

Pierwsza implementacja powinna transformować dane podczas podawania ich do ImPlot (getter/callback lub odpowiadająca temu publiczna ścieżka API w używanej wersji), bez utrzymywania drugiej pełnej kopii znormalizowanych danych.

Jeżeli pomiary wykażą koszt, później można porównać wariant z cache'em.

### Fit Y

`Fit Y` działa tylko dla aktywnej widocznej serii i:

- bierze `dataMin/dataMax` z całej serii, niezależnie od aktualnego viewportu X;
- nadpisuje wcześniejszy `viewMin/viewMax`;
- dodaje około 5% marginesu.

### Pan Y

`Alt + drag` przesuwa `viewMin/viewMax` aktywnej widocznej serii.

Serię można przesunąć całkowicie poza ekran. Nie wprowadzamy sztucznych limitów.

Podczas tej operacji X pozostaje nieruchomy.

### Zoom Y

`Alt + wheel` skaluje `viewMin/viewMax` aktywnej widocznej serii.

Zoom jest centrowany wokół środka aktualnego zakresu `viewMin/viewMax`, nie wokół kursora myszy.

Trzeba zabezpieczyć zakres przed degeneracją do zera.

### Zachowanie po zmianie aktywnej serii

Zmiana active nie resetuje Y. Powrót do wcześniej aktywnej serii przywraca jej wcześniejszy `viewMin/viewMax`.

## 10. Aktywna seria

Może istnieć zero lub jedna aktywna seria.

Po Reset nie ma aktywnej serii.

Aktywna seria:

- jest rysowana na końcu;
- właściwa linia danych ma **1.5 px** i zachowuje pełny własny kolor;
- pod właściwą linią ma osobną warstwę wyróżnienia przełączaną w `Test Controls`;
- `Halo` ma około **7 px**, używa koloru serii i alpha około **0.22**;
- `Outline` ma około **5 px**, używa neutralnego koloru tekstu bieżącego stylu i alpha około **0.65**;
- domyślnym trybem jest `Halo`;
- właściwa linia danych pozostaje renderowana przez ImPlot;
- Halo/Outline jest wizualnym overlayem rysowanym jako jedna połączona screen-space polyline przez publiczny `ImDrawList::AddPolyline`, aby zachować poprawne łączenia grubej linii na załamaniach;
- do overlayu wystarczają punkty z aktualnie widocznego zakresu X plus niewielki zapas poza krawędziami plot area;
- warstwa Halo/Outline nie tworzy dodatkowej pozycji w native legend, nie wpływa na Fit i nie zmienia geometrii hit-testu;
- jako jedyna widoczna seria prezentuje semantyczną oś Y i kolorowy grid Y.

Pozostałe widoczne serie:

- linia 1 px;
- pełny kolor, bez wygaszania.

Jeśli aktywna seria zostanie ukryta:

- logicznie nadal pozostaje active;
- nie ma aktywnej **widocznej** serii;
- semantyczna oś Y jest pusta;
- kolorowy grid active znika;
- Fit Y / Alt-pan / Alt-wheel są nieaktywne.

## 11. Wybór serii przez RMB i hit-test

RMB na linii służy do wyboru aktywnej serii.

Reguły:

- hit-test tylko widocznych serii;
- tolerancja około 5 px;
- przy kilku kandydatach wygrywa najbliższa geometria w pikselach;
- RMB na nieaktywnej serii ustawia ją jako active;
- RMB na aktualnie aktywnej serii wyłącza active;
- RMB poza wszystkimi liniami czyści active.

Hit-test powinien odpowiadać faktycznie widocznej, przetransformowanej geometrii.

Przy dużym zoom-out nie wystarczy sprawdzenie tylko próbki najbliższej czasowo. Algorytm powinien:

- przeliczyć poziome około 5 px wokół myszy na przedział czasu;
- z regularnego `dt` wyznaczyć odpowiedni zakres indeksów;
- sprawdzić wszystkie istotne segmenty w tym lokalnym zakresie;
- transformować ich Y zgodnie z `viewMin/viewMax` danej serii;
- liczyć odległość point-to-segment w screen space;
- wybrać najbliższy segment w tolerancji.

Na początku bez dodatkowego indeksu/LOD. Optymalizacja dopiero, jeśli Stress Raw tego wymaga.

## 12. Oś Y aktywnej serii

Widoczna semantyczna oś Y jest jedna, po lewej stronie i dotyczy tylko aktywnej widocznej serii.

- fizyczna oś ImPlot pozostaje `0..1` i ma wyłączone natywne dekoracje Y;
- aplikacja zawsze rezerwuje po lewej stały gutter semantycznej osi Y, niezależnie od tego, czy istnieje active series;
- aktywacja, zmiana lub wyłączenie active nie może zmieniać `plotPos` ani `plotSize`;
- szerokość guttera jest wyliczana z bieżącego fontu i stałego referencyjnego budżetu etykiety, a nie z aktualnych wartości serii;
- gdy nie ma aktywnej widocznej serii, gutter pozostaje zarezerwowany, ale jest pusty;
- etykiety i tick marks aktywnej serii rysuje aplikacja w tym gutterze przez publiczny ImGui DrawList;
- etykiety są generowane w jednostkach surowych aktywnej serii;
- surowe wartości ticków są transformowane do pozycji `0..1`;
- liczba ticków jest adaptowana do wysokości wykresu;
- nazwa aktywnej serii jest pokazana poziomo nad lub przy osi Y;
- obszar osi ma stałą, przewidywalną szerokość.

Format liczb:

- domyślnie 2 miejsca po przecinku;
- jeśli globalny span serii `dataMax - dataMin < 1.0`, domyślnie 3 miejsca;
- jeśli liczba nie mieści się w stałej szerokości osi, można użyć formatu naukowego/skróconego.

Ta sama podstawowa reguła formatowania dotyczy `Values`.

Oś Y opisuje bieżący `viewMin..viewMax`, nawet jeśli rzeczywisty globalny zakres danych zajmuje tylko część widoku.

## 13. Grid

### Neutralny grid bazowy

Zawsze obecny, subtelny szary grid.

Pionowy:

- major dokładnie na opisanych tickach X;
- opcjonalny lżejszy minor bez etykiet.

Poziomy:

- neutralne podziały wizualne;
- nie reprezentują wartości konkretnej serii;
- liczba linii adaptowana do wysokości wykresu.

### Kolorowy grid aktywnej serii

Jeżeli istnieje aktywna widoczna seria, dodatkowo rysowane są poziome linie w jej kolorze.

- tylko poziome, nigdy pionowe;
- odpowiadają semantycznym „nice” wartościom Y;
- liczba/odstęp adaptowane do wysokości;
- ticki są pozycjonowane zgodnie z aktualnym `viewMin/viewMax`;
- kolorowy grid jest ograniczony do globalnego rzeczywistego zakresu `dataMin..dataMax` całej serii po zastosowaniu aktualnej transformacji Y;
- jeśli część globalnego zakresu po pan Y znajduje się poza ekranem, widoczna jest tylko część kolorowego grida mieszcząca się w plot area;
- semantyczna oś Y nadal opisuje cały aktualny `viewMin..viewMax`.

## 14. Widoczność serii

Ukryta seria:

- nie jest przekazywana do funkcji rysującej linię;
- zachowuje dane, kolor, active flag i stan Y;
- nie uczestniczy w hit-test RMB;
- nie pojawia się w `Values`.

Po ponownym pokazaniu wraca z poprzednim `viewMin/viewMax`.

## 15. Kolory

W obrębie datasetu każda seria powinna mieć własny deterministyczny kolor.

Nie polegamy na krótkiej domyślnej palecie, która zacznie powtarzać barwy przy 60 seriach.

Dopuszczalna jest własna deterministyczna paleta oparta np. na rozłożeniu hue.

## 16. Legendy

Eksperyment ma pozwalać porównać oba warianty w czasie działania.

### Native ImPlot Legend

- opcjonalnie widoczna;
- informacyjna;
- bez przycisków hide/show;
- nie służy do wyboru active;
- nie wymuszamy pełnej synchronizacji zachowania z Custom Legend;
- może pokazywać tylko aktualnie renderowane serie.

Nie używamy `implot_internal.h` tylko po to, aby przejąć wewnętrzny stan legendy.

### Custom Legend

Pływające okno ImGui.

- pokazuje wszystkie serie, także ukryte;
- posiada kontrolę visibility każdej serii;
- kliknięcie nazwy serii przełącza active: inactive -> active, active -> brak active;
- kliknięcie nazwy ukrytej serii może ustawić ją jako active, ale nie pokazuje jej automatycznie;
- kolejność serii jest stała.

## 17. Values i crosshair

`Values` jest osobnym pływającym oknem ImGui.

Pokazuje wszystkie **widoczne** serie w stałej kolejności.

Aktywna seria pozostaje na swojej pozycji i jest wyróżniona tylko pogrubionym tekstem.

Przy każdej serii pokazujemy kolorowy znacznik/swath.

Na górze pokazujemy aktualny czas myszy.

### Odczyt pod myszą

- działa tylko, gdy mysz jest wewnątrz plot area i wewnątrz czasu datasetu;
- po wyjściu z plot area odczyt jest czyszczony;
- przed pierwszą lub po ostatniej próbce — brak odczytu;
- wartości są interpolowane liniowo pomiędzy sąsiednimi próbkami regularnego X;
- ukrytych serii nie interpolujemy ani nie pokazujemy.

Czas w Values ma stały czytelny format z częścią milisekundową, np. `mm:ss.mmm`, a po przekroczeniu godziny `hh:mm:ss.mmm`. Dokładny wizualny wariant można później skorygować po ręcznym teście.

### Crosshair

Crosshair to tylko pionowa linia czasu.

- podąża za dokładnym mouse X;
- nie snapuje do próbki;
- można go wyłączyć w `Test Controls`;
- wyłączenie linii nie wyłącza działania `Values`.

## 18. Kursory A/B

Są dokładnie dwa kursory: A i B.

- reprezentują czas, nie pozycję ekranową;
- po pan/zoom X pozostają przy tym samym czasie;
- startują ukryte;
- mają różne stałe, kontrastowe kolory;
- linia około 2 px;
- etykieta `A` / `B` przy górnej krawędzi plot area;
- na początku bez rozwiązywania kolizji etykiet.

Ustawianie:

- `Shift + LMB` — ustaw/przenieś A dokładnie do czasu kliknięcia;
- `Ctrl + LMB` — ustaw/przenieś B.

Te gesty mają pierwszeństwo nad rozpoczęciem zwykłego drag elementu.

Istniejący kursor można przeciągać **wyłącznie zwykłym LMB bez modyfikatora**. `Alt` nie jest wariantem drag kursora.

Pozycja aktualizuje się w każdej klatce podczas drag.

Czas jest clampowany do `0..duration`.

`Ctrl + RMB` na linii kursora ukrywa/usuwa dany kursor.

Pierwsza implementacja może korzystać z publicznego mechanizmu drag line ImPlot, jeżeli odpowiada wymaganej semantyce.

## 19. Markery

Marker jest pionową linią w konkretnym czasie.

- kilka markerów, nie setki;
- numeracja `0, 1, 2, ...`;
- po usunięciu numer nie jest ponownie używany;
- tylko pełny Reset zeruje licznik;
- stały wspólny kolor markerów, różny od A/B;
- linia około 1 px;
- zawsze widoczna etykieta numeru przy górnej krawędzi;
- bez selected state;
- bez collision avoidance etykiet w pierwszej wersji.

Dodawanie:

- `Alt + LMB` traktowane jako klik — tworzy marker;
- marker nie snapuje do próbki;
- czas jest clampowany do datasetu.

Przeciąganie:

- zwykły LMB na markerze;
- update w czasie rzeczywistym.

Usuwanie:

- `Ctrl + RMB` na markerze.

Konflikt `Alt + click` z `Alt + drag` rozwiązujemy progiem ruchu około 4–5 px:

- poniżej progu — marker;
- po przekroczeniu progu — pan Y i brak nowego markera.

`Alt + drag` jest zarezerwowany dla pan Y i nie przeciąga istniejącego kursora ani markera. Istniejące elementy przeciąga się plain LMB.

## 20. Priorytety wejścia

Docelowa mapa:

1. `Shift + LMB` — set A, bez rozpoczęcia drag innego elementu.
2. `Ctrl + LMB` — set B.
3. `Alt + LMB` click — add marker.
4. `Alt + drag` — pan Y active visible series; X nieruchomy; cursor/marker drag jest przy `Alt` wyłączony.
5. `Alt + wheel` — zoom Y active visible series; X nieruchomy.
6. plain LMB drag na cursor/marker — drag elementu.
7. plain LMB drag poza narzędziem — natywny pan X.
8. plain wheel — natywny zoom X.
9. `Ctrl + RMB` na cursor/marker — usuń/ukryj jeden element; ma pierwszeństwo nad wyborem serii.
10. plain RMB — hit-test serii i toggle/clear active.

Jeżeli cursor i marker leżą na sobie, wystarczy że można złapać/usunąć jeden z nich. Nie wymagamy deterministycznego wyboru konkretnego typu w tej kolizji.

Natywne menu kontekstowe/akcje RMB ImPlot nie mogą kolidować z RMB używanym do wyboru serii.

Największym ryzykiem integracyjnym do praktycznego sprawdzenia jest własność wejścia pomiędzy natywną nawigacją ImPlot a naszymi narzędziami Alt/Shift/Ctrl.

## 21. Render order

Docelowo:

1. neutralny grid;
2. nieaktywne widoczne serie;
3. screen-space warstwa Halo/Outline aktywnej widocznej serii;
4. cienka właściwa linia aktywnej widocznej serii;
5. kolorowy grid/warstwa semantyczna według rozwiązania zapewniającego poprawny efekt wizualny;
6. cursory/markery/crosshair i etykiety jako warstwy overlay.

Dokładny porządek grida względem linii można skorygować wizualnie podczas testów. Ważne jest zachowanie semantyki i czytelności.

## 22. Weryfikacja

Projekt rozdziela trzy rodzaje weryfikacji:

1. **functional C++** — GoogleTest + CTest dla logiki, matematyki, modelu i edge-case'ów, które nie wymagają GUI;
2. **GUI integration** — Dear ImGui Test Engine dla rzeczywistych widgetów, gestów ImPlot i routingu wejścia;
3. **manual** — wygląd, czytelność, ergonomia i subiektywna płynność.

Testy nie mają wymuszać osobnej architektury produkcyjnej. Najpierw testujemy istniejący interfejs; małą czystą funkcję/model wydzielamy tylko wtedy, gdy eliminuje to duplikowanie algorytmu lub kruchy test GUI i ma sens również dla kodu produkcyjnego.

Nie przenosimy automatycznie narzędzi testowych z poprzedniego wxWidgets GUI. OS-level automatyzacja może zostać dodana dopiero dla konkretnej luki, której Dear ImGui Test Engine nie potrafi sprawdzić.

Szczegółowy aktualny plan, wyniki i merge policy znajdują się w `docs/AUTOMATED_TESTS.md`.

## 23. Kryterium sukcesu eksperymentu

Eksperyment nie ma z góry udowodnić, że ImPlot jest właściwym wyborem.

Po implementacji powinniśmy móc ocenić:

- czy niezależne Y jest naturalne czy wymaga zbyt dużej warstwy obejściowej;
- czy własny time axis i semantic Y są stabilne;
- czy hit-test i narzędzia myszy dają przewidywalny UX;
- czy native/custom legend przynoszą realną wartość;
- czy raw rendering Reference datasetu jest praktycznie używalny;
- gdzie zaczyna się problem wydajności;
- ile kodu specyficznego dla ImPlot powstało;
- które elementy były łatwiejsze lub trudniejsze niż w obecnym podejściu CANStatio.

Wynik może być pozytywny, negatywny lub mieszany. Prototyp powinien dostarczyć danych do decyzji, a nie wymuszać konkretny wniosek.
