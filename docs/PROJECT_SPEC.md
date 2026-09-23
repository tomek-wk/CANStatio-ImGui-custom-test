# Specyfikacja eksperymentu CANStatio ImGui Custom Chart Test

## 1. Cel

Projekt jest samodzielnym prototypem do sprawdzenia Dear ImGui z własną implementacją wykresu jako potencjalnej warstwy wykresu dla CANstatio LogViewer.

Nie testujemy całej aplikacji LogViewer. Interesuje nas wykres oraz funkcje bezpośrednio związane z jego obsługą.

Główne pytania eksperymentu:

- czy własny renderer wykresu w obrębie Dear ImGui daje wystarczającą kontrolę nad wieloma seriami z niezależnym Y i wspólnym X;
- ile kodu wymaga pełne przejęcie layoutu, renderingu, hit-testu i inputu;
- czy własne interakcje pozostają przewidywalne przy nakładających się narzędziach;
- jaka jest praktyczna responsywność przy danych zbliżonych do planowanego LogViewera;
- czy koszt kodu i utrzymania jest akceptowalny.

Priorytetem jest funkcjonalność, przewidywalność UX i koszt implementacji. Wydajność jest obserwowana, ale na starcie nie definiujemy twardego progu FPS.

## 2. Zakres i rzeczy świadomie wykluczone

W zakresie:

- generowane dane w pamięci;
- wiele serii na jednym wykresie;
- wspólna oś czasu X;
- niezależny Y każdej serii;
- aktywna seria;
- widoczność serii;
- własny plot area;
- własne osie i grid;
- własne transformacje data/screen;
- własny pan/zoom X;
- własny pan/zoom Y aktywnej serii;
- Fit X i Fit Y;
- wybór serii przez trafienie w linię;
- Custom Legend;
- Values pod myszą;
- crosshair czasu;
- kursory A/B;
- markery czasu;
- ręczna obserwacja FPS i frame time;
- zestawy danych do testów funkcjonalnych i stress testu;
- automatyczne testy funkcyjne logiki C++;
- automatyczne testy integracyjne GUI Dear ImGui.

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
- dodatkowa biblioteka wykresowa;
- zewnętrzna OS-level automatyzacja GUI, dopóki Dear ImGui Test Engine wiarygodnie pokrywa bieżące potrzeby.

## 3. Stack

- C++20
- Windows jako pierwsza platforma
- GCC / MinGW-w64 z MSYS2 UCRT64
- CMake + Ninja
- GLFW
- OpenGL 3.3 Core Profile
- Dear ImGui
- Dear ImGui Test Engine dla integracyjnych testów GUI
- GoogleTest + CTest dla testów funkcyjnych C++

Dear ImGui, GLFW i Dear ImGui Test Engine są pobierane przez `FetchContent` z przypiętych referencji w `CMakeLists.txt`.

GoogleTest jest używany jako istniejący lokalny pakiet znaleziony przez `find_package(GTest REQUIRED)`; projekt nie pobiera go przez `FetchContent`.

Bazowe wersje eksperymentu:

- Dear ImGui `v1.92.9b`
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
- przełączniki widoczności Values i Custom Legend;
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

## 8. Plot area i transformacje współrzędnych

Aplikacja jest właścicielem prostokąta plot area.

Dla aktualnego zakresu X:

```text
screenX = plotLeft + (time - xMin) / (xMax - xMin) * plotWidth
```

Dla każdej serii niezależnie:

```text
screenY = plotBottom - (value - viewMin) / (viewMax - viewMin) * plotHeight
```

Transformacje odwrotne służą do obsługi myszy, hit-testu i nawigacji.

Nie istnieje wymaganie wspólnej sztucznej przestrzeni Y dla wszystkich serii. Każda seria jest mapowana bezpośrednio ze swojego surowego zakresu `viewMin..viewMax` do tego samego plot area.

Wartości poza aktualnym zakresem mogą wypaść poza plot area i są przycinane przez clipping renderera.

Plot area ma stabilną geometrię przy zmianie active series. Etykiety i dekoracje osi są zarządzane przez aplikację tak, aby zmiana zakresu lub aktywnej serii nie powodowała skoków szerokości wykresu.

## 9. Oś czasu X

X jest czasem względnym od początku datasetu. Początek to `00`, bez absolutnej daty/godziny.

Oś X:

- zawsze na dole;
- bez tytułu `Time`;
- etykiety pod pionowymi liniami grida;
- format adaptowany do aktualnego zoomu;
- po przekroczeniu godziny używa godzin.

Przykładowe formaty zależnie od skali:

- sekundy;
- `mm:ss`;
- `hh:mm:ss`;
- dokładniejszy format z częścią milisekundową po dużym zoomie.

Ticki X generuje aplikacja na podstawie:

- `xMin/xMax`;
- szerokości plot area w pikselach;
- zestawu czytelnych interwałów czasu, np. 100 ms, 500 ms, 1 s, 5 s, 10 s, 30 s, 1 min, 5 min itd.

Główne pionowe linie grida odpowiadają opisanym tickom czasu. Dopuszczalne są lżejsze linie minor bez etykiet.

Nawigacja X jest w pełni app-owned:

- zwykły LMB drag poza przejętym narzędziem — pan X;
- zwykłe kółko — zoom X;
- `Fit X` — pełny czas datasetu.

Na tym etapie nie wymagamy clampa viewportu X do granic datasetu. Kursory i markery są natomiast clampowane do datasetu.

## 10. Niezależny Y

Każda seria utrzymuje własny stan:

- `viewMin`;
- `viewMax`.

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

Zakres musi być zabezpieczony przed degeneracją do zera.

Zmiana active nie resetuje Y. Powrót do wcześniej aktywnej serii przywraca jej wcześniejszy `viewMin/viewMax`.

## 11. Aktywna seria

Może istnieć zero lub jedna aktywna seria.

Po Reset nie ma aktywnej serii.

Aktywna seria:

- jest rysowana na końcu;
- właściwa linia danych ma około **1.5 px** i zachowuje pełny własny kolor;
- może mieć osobną warstwę wyróżnienia przełączaną w `Test Controls`;
- `Halo` ma około **7 px**, używa koloru serii i alpha około **0.22**;
- `Outline` ma około **5 px**, używa neutralnego koloru tekstu bieżącego stylu i alpha około **0.65**;
- domyślnym trybem jest `Halo`;
- highlight i właściwa linia korzystają z tej samej app-owned geometrii screen-space;
- highlight nie zmienia hit-testu.

Pozostałe widoczne serie mają linię około 1 px i pełny kolor.

Jeśli aktywna seria zostanie ukryta:

- logicznie nadal pozostaje active;
- nie ma aktywnej widocznej serii;
- semantyczna oś Y jest pusta;
- kolorowy grid active znika;
- Fit Y / Alt-pan / Alt-wheel są nieaktywne.

## 12. Wybór serii przez RMB i hit-test

RMB na linii służy do wyboru aktywnej serii.

Reguły:

- hit-test tylko widocznych serii;
- tolerancja około 5 px;
- przy kilku kandydatach wygrywa najbliższa geometria w pikselach;
- RMB na nieaktywnej serii ustawia ją jako active;
- RMB na aktualnie aktywnej serii wyłącza active;
- RMB poza wszystkimi liniami czyści active.

Hit-test odpowiada faktycznie widocznej geometrii własnego renderera.

Przy dużym zoom-out algorytm powinien:

- przeliczyć poziome około 5 px wokół myszy na przedział czasu;
- z regularnego `dt` wyznaczyć odpowiedni zakres indeksów;
- sprawdzić wszystkie istotne segmenty w tym lokalnym zakresie;
- mapować ich wartości do screen-space zgodnie z aktualnym X i własnym Y serii;
- liczyć odległość point-to-segment w pikselach;
- wybrać najbliższy segment w tolerancji.

Na początku bez dodatkowego indeksu/LOD. Optymalizacja dopiero, jeśli Stress Raw tego wymaga.

## 13. Oś Y i grid

Widoczna semantyczna oś Y jest jedna, po lewej stronie i dotyczy tylko aktywnej widocznej serii.

- wartości etykiet wynikają z bieżącego `viewMin..viewMax` aktywnej serii;
- liczba ticków jest adaptowana do wysokości plot area;
- format domyślnie używa 2 miejsc po przecinku;
- jeśli globalny span serii `dataMax - dataMin < 1.0`, domyślnie używa 3 miejsc;
- przy bardzo dużych lub małych liczbach można użyć formatu naukowego/skróconego;
- nazwa aktywnej serii jest pokazana przy osi lub w górnej części plot area.

Layout osi jest app-owned i ma być przewidywalny. Zmiana active series nie może przesuwać samego plot area.

### Neutralny grid

Zawsze obecny, subtelny szary grid.

Pionowe linie major odpowiadają opisanym tickom X. Poziome neutralne podziały nie reprezentują wartości konkretnej serii.

### Kolorowy grid aktywnej serii

Jeżeli istnieje aktywna widoczna seria, dodatkowo rysowane są poziome linie w jej kolorze.

- odpowiadają czytelnym wartościom Y aktywnej serii;
- pozycja wynika z jej aktualnego `viewMin/viewMax`;
- semantyczna oś Y opisuje cały bieżący zakres widoku.

## 14. Widoczność i kolory serii

Ukryta seria:

- nie jest renderowana;
- zachowuje dane, kolor, active state i Y state;
- nie uczestniczy w hit-test RMB;
- nie pojawia się w `Values`.

Po ponownym pokazaniu wraca z poprzednim `viewMin/viewMax`.

W obrębie datasetu każda seria ma własny deterministyczny kolor. Nie polegamy na krótkiej domyślnej palecie, która zacznie powtarzać barwy przy 60 seriach.

## 15. Custom Legend

Custom Legend jest pływającym oknem ImGui.

- pokazuje wszystkie serie, także ukryte;
- steruje visibility każdej serii;
- kliknięcie nazwy serii przełącza active: inactive -> active, active -> brak active;
- kliknięcie nazwy ukrytej serii może ustawić ją jako active, ale nie pokazuje jej automatycznie;
- kolejność serii jest stała.

Nie planujemy drugiej, renderer-owned legendy.

## 16. Values i crosshair

`Values` jest osobnym pływającym oknem ImGui.

Pokazuje wszystkie widoczne serie w stałej kolejności. Aktywna seria pozostaje na swojej pozycji i jest wyróżniona tylko pogrubionym tekstem. Przy każdej serii pokazujemy kolorowy znacznik.

### Odczyt pod myszą

- działa tylko, gdy mysz jest wewnątrz plot area i wewnątrz czasu datasetu;
- po wyjściu z plot area odczyt jest czyszczony;
- przed pierwszą lub po ostatniej próbce — brak odczytu;
- wartości są interpolowane liniowo pomiędzy sąsiednimi próbkami regularnego X;
- ukrytych serii nie interpolujemy ani nie pokazujemy.

Czas w Values ma czytelny format z częścią milisekundową, np. `mm:ss.mmm`, a po przekroczeniu godziny `hh:mm:ss.mmm`.

### Crosshair

Crosshair to pionowa linia czasu.

- podąża za dokładnym mouse X;
- nie snapuje do próbki;
- można go wyłączyć w `Test Controls`;
- wyłączenie linii nie wyłącza działania `Values`.

## 17. Kursory A/B

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

Istniejący kursor można przeciągać wyłącznie zwykłym LMB bez modyfikatora.

Pozycja aktualizuje się w każdej klatce podczas drag.

Czas jest clampowany do `0..duration`.

`Ctrl + RMB` na linii kursora ukrywa dany kursor.

Render, hit-test i drag kursora są realizowane przez wspólny system współrzędnych własnego wykresu.

## 18. Markery

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

## 19. Priorytety wejścia

Docelowa mapa:

1. `Shift + LMB` — set A, bez rozpoczęcia drag innego elementu.
2. `Ctrl + LMB` — set B.
3. `Alt + LMB` click — add marker.
4. `Alt + drag` — pan Y active visible series; X nieruchomy; cursor/marker drag przy `Alt` wyłączony.
5. `Alt + wheel` — zoom Y active visible series; X nieruchomy.
6. plain LMB drag na cursor/marker — drag elementu.
7. plain LMB drag poza narzędziem — pan X.
8. plain wheel — zoom X.
9. `Ctrl + RMB` na cursor/marker — usuń/ukryj jeden element; ma pierwszeństwo nad wyborem serii.
10. plain RMB — hit-test serii i toggle/clear active.

Jeżeli cursor i marker leżą na sobie, wystarczy że można złapać/usunąć jeden z nich. Nie wymagamy deterministycznego wyboru konkretnego typu w tej kolizji.

Routing wejścia jest całkowicie app-owned.

## 20. Render order

Docelowo:

1. tło plot area;
2. neutralny grid;
3. nieaktywne widoczne serie;
4. highlight aktywnej widocznej serii;
5. cienka właściwa linia aktywnej widocznej serii;
6. kolorowy grid/warstwa semantyczna według rozwiązania zapewniającego najlepszą czytelność;
7. kursory, markery, crosshair i etykiety jako warstwy overlay;
8. dekoracje osi i tekst wymagający rysowania ponad plot area.

Dokładny porządek grida względem linii można skorygować wizualnie podczas testów. Ważne jest zachowanie semantyki i czytelności.

## 21. Rendering i clipping

Pierwsza implementacja ma być możliwie prosta:

- bez LOD;
- bez pełnego cache'a drugiej kopii geometrii;
- punkty widocznych serii są transformowane do screen-space podczas renderowania;
- rysowanie korzysta z clippingu do plot area;
- można ograniczyć przetwarzanie do indeksów odpowiadających aktualnemu zakresowi X plus niewielki zapas na krawędziach.

Optymalizacje wprowadzamy dopiero po pomiarze Reference i Stress Raw.

## 22. Weryfikacja

Projekt rozdziela trzy rodzaje weryfikacji:

1. **functional C++** — GoogleTest + CTest dla logiki, transformacji, stanu, matematyki i edge-case'ów;
2. **GUI integration** — Dear ImGui Test Engine dla rzeczywistych widgetów i routingu wejścia;
3. **manual** — wygląd, czytelność, ergonomia i subiektywna płynność.

Testy nie mają wymuszać osobnej architektury produkcyjnej. Małą czystą funkcję lub model wydzielamy tylko wtedy, gdy eliminuje to duplikowanie algorytmu albo kruchy test GUI i ma sens również dla kodu produkcyjnego.

## 23. Kryterium sukcesu eksperymentu

Po implementacji powinniśmy móc ocenić:

- czy niezależne Y jest naturalne w własnym rendererze;
- czy time axis i semantic Y są stabilne i proste w utrzymaniu;
- czy hit-test i narzędzia myszy dają przewidywalny UX;
- czy jeden wspólny system data/screen upraszcza Values, selection, cursory i markery;
- czy raw rendering Reference datasetu jest praktycznie używalny;
- gdzie zaczyna się problem wydajności;
- ile kodu wymaga własny renderer i input routing;
- czy większa kontrola nad zachowaniem wykresu uzasadnia koszt utrzymania.

Wynik może być pozytywny, negatywny lub mieszany. Prototyp ma dostarczyć danych do późniejszej decyzji o warstwie wykresów LogViewera.