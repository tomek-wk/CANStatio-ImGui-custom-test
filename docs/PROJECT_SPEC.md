# Specyfikacja eksperymentu CANStatio ImGui Custom Chart Test

## 1. Cel

Projekt jest samodzielnym prototypem do sprawdzenia Dear ImGui z własną implementacją wykresu jako potencjalnej warstwy wykresu dla CANstatio LogViewer.

Nie testujemy całej aplikacji LogViewer. Interesuje nas wykres oraz funkcje bezpośrednio związane z jego obsługą.

Eksperyment ma odpowiedzieć przede wszystkim na pytania:

- czy własny renderer daje wystarczającą kontrolę nad wieloma seriami z niezależnym Y i wspólnym X;
- ile kodu wymaga pełne przejęcie layoutu, renderingu, hit-testu i inputu;
- czy własne interakcje są przewidywalne przy nakładających się narzędziach;
- jaka jest praktyczna responsywność przy danych zbliżonych do planowanego LogViewera;
- czy koszt kodu i utrzymania jest akceptowalny.

Priorytetem jest funkcjonalność, prostota, przewidywalność UX i koszt implementacji. Wydajność obserwujemy, ale na starcie nie definiujemy twardego progu FPS.

## 2. Zakres

W zakresie pierwszego eksperymentu:

- generowane dane w pamięci;
- wiele serii na jednym wykresie;
- wspólna regularna oś czasu X;
- niezależny stan Y każdej serii;
- własny plot area;
- własne transformacje data/screen;
- własne osie, ticki, etykiety i grid;
- własny pan/zoom X;
- własny pan/zoom Y aktywnej serii;
- Fit X i Fit Y;
- aktywna seria i visibility;
- wybór serii przez hit-test linii;
- Custom Legend;
- Values pod myszą;
- crosshair czasu;
- kursory A/B;
- markery czasu;
- jednoznaczny routing inputu;
- ręczna obserwacja FPS i frame time;
- presety funkcjonalne i stress test;
- functional tests C++;
- GUI integration tests Dear ImGui.

Poza zakresem pierwszej wersji:

- parser plików i CSV;
- DataCore i format projektu LogViewera;
- irregular sampling i osobne timestampy każdej serii;
- NaN/gaps i specjalna semantyka przerw;
- sygnały step/digital;
- Overview/minimapa;
- LOD/downsampling, dopóki pomiar nie pokaże realnej potrzeby;
- integracja z głównym CANstatio;
- dodatkowa biblioteka wykresowa;
- zewnętrzna OS-level automatyzacja GUI, dopóki Dear ImGui Test Engine pokrywa potrzeby.

## 3. Stack

- C++20;
- Windows jako pierwsza platforma;
- GCC / MinGW-w64 z MSYS2 UCRT64;
- CMake + Ninja;
- GLFW;
- OpenGL 3.3 Core Profile;
- Dear ImGui;
- Dear ImGui Test Engine;
- GoogleTest + CTest.

Przypięte wersje bazowe:

- Dear ImGui `v1.92.9b`;
- GLFW `3.5.1`;
- Dear ImGui Test Engine commit `508a8fc8dacac2f346d353fed31b9bc90ed29adc`.

Dear ImGui, GLFW i Test Engine są pobierane przez `FetchContent`. GoogleTest jest lokalnym/systemowym pakietem znalezionym przez `find_package(GTest REQUIRED)`.

## 4. Główne okno i UI pomocniczy

Wykres ma wizualnie działać jak główna powierzchnia aplikacji, a nie mały widget w klasycznym oknie ImGui.

- główna powierzchnia ImGui zajmuje cały client area GLFW;
- wykres automatycznie wypełnia dostępne miejsce;
- resize nie zachowuje stałego aspect ratio;
- marginesy są małe;
- brak dockingu na tym etapie.

Aplikacja obsługuje docelowy tryb **exclusive fullscreen**:

- `F11` przełącza zwykłe okno i exclusive fullscreen;
- fullscreen używa bieżącego trybu wideo głównego monitora;
- wejście w fullscreen nie resetuje datasetu ani stanu wykresu;
- wyjście z fullscreen przywraca poprzednią pozycję i rozmiar okna.

Pływające okna pomocnicze:

- `Test Controls`;
- `Values`;
- `Custom Legend`.

Można je przesuwać i ukrywać. Zamknięcie nie niszczy stanu. `Test Controls` pozwala je ponownie pokazać.

`Test Controls` docelowo zawiera co najmniej:

- wybór presetu;
- `Fit X`;
- `Fit Y`;
- `Reset`;
- wybór `Halo` / `Outline`;
- visibility Values i Custom Legend;
- crosshair on/off;
- metadane datasetu;
- FPS i frame time.

## 5. Model danych

W pierwszej wersji wszystkie serie mają wspólną regularną oś czasu.

Dataset:

- `N` — liczba próbek na serię;
- `dt` — stały krok czasu;
- czas próbki `i`: `x = i * dt`;
- seria przechowuje `std::vector<float>` wartości Y;
- brak osobnej tablicy timestampów dla każdej serii.

Generator jest deterministyczny. Ten sam preset daje te same dane przy każdym uruchomieniu.

Każda seria ma co najmniej:

- nazwę;
- deterministyczny kolor;
- `visible`;
- `dataMin` / `dataMax`;
- `viewMin` / `viewMax`;
- wartości Y.

## 6. Presety generatora

1. **Small / Sanity** — 3 serie × 1 000 próbek, `dt = 100 ms`.
2. **Reference** — 60 × 36 000, `dt = 100 ms`, 1 godzina.
3. **Overlap** — 60 × 36 000, dużo przecinających i nakładających się przebiegów.
4. **Mixed Scale** — 60 × 36 000, bardzo różne zakresy, np. `0..8000`, `0..100`, `-1..1`, `100.001..100.009`.
5. **Spikes / Noise** — 60 × 36 000, szum, szybkie zmiany i izolowane piki.
6. **Long Time** — około 10 × 108 000, `dt = 100 ms`, 3 godziny.
7. **Stress Raw** — 60 × 360 000, `dt = 10 ms`, 1 godzina, 21.6 mln wartości.

`Stress Raw` służy do znalezienia granicy. Pierwsza implementacja renderuje raw data bez LOD.

## 7. Reset i zmiana datasetu

`Reset` oraz zmiana presetu prowadzą do tego samego stanu początkowego:

- X = pełny dataset `0..duration`;
- wszystkie serie widoczne;
- brak active;
- każda seria ma Fit Y z całego datasetu z około 5% marginesu;
- kursory A/B ukryte;
- wszystkie markery usunięte;
- licznik markerów = `0`.

Dla serii stałej tworzony jest mały dodatni zakres wokół wartości, aby uniknąć zakresu zerowego.

Usunięty numer markera nie jest używany ponownie. Licznik zeruje tylko pełny Reset.

## 8. Plot area i transformacje

Aplikacja jest właścicielem prostokąta plot area oraz wszystkich transformacji.

Dla X:

```text
screenX = plotLeft + (time - xMin) / (xMax - xMin) * plotWidth
```

Dla Y konkretnej serii:

```text
screenY = plotBottom - (value - viewMin) / (viewMax - viewMin) * plotHeight
```

Istnieją również transformacje odwrotne screen-to-data.

Nie istnieje wspólna sztuczna przestrzeń Y. Każda seria mapuje własne `viewMin..viewMax` bezpośrednio do tego samego plot area.

Wartości poza viewportem są naturalnie clippingowane.

Plot area ma stabilną geometrię przy zmianie active series i zmianach wartości etykiet osi.

Zakresy X/Y muszą zawsze być skończone i mieć dodatni span. Zabezpieczenie przed degeneracją jest technicznym, małym epsilonem zależnym od skali; nie jest to użytkowy limit nawigacji. Jeżeli stan stanie się niefinity lub niepoprawny, X wraca do Fit X, a Y danej serii do Fit Y.

## 9. Oś czasu X i grid X

X jest czasem względnym od początku datasetu. Początek to `00`; nie ma absolutnej daty/godziny.

Oś X:

- zawsze na dole;
- bez tytułu `Time`;
- etykiety pod głównymi pionowymi liniami grida;
- format adaptowany do zoomu;
- po przekroczeniu godziny używa godzin.

Typowe formaty:

- sekundy;
- `mm:ss`;
- `hh:mm:ss`;
- format z milisekundami przy dużym zoomie.

Ticki X generuje aplikacja z `xMin/xMax`, szerokości plot area i czytelnego zestawu interwałów czasu, np. `100 ms`, `500 ms`, `1 s`, `5 s`, `10 s`, `30 s`, `1 min`, `5 min` itd.

Major grid X odpowiada opisanym tickom. Minor grid może być dodany bez etykiet.

## 10. Nawigacja X

Nawigacja X jest całkowicie app-owned i działa tylko nad plot area, jeżeli gesture nie został przejęty przez narzędzie.

### Pan X

- plain LMB drag poza kursorem/markerem przesuwa X;
- zachowanie jest typu „grab content”: przeciągany wykres podąża za myszą;
- viewport X nie jest clampowany do datasetu.

### Zoom X

- plain wheel nad plot area wykonuje zoom X;
- punktem kotwiczenia jest czas znajdujący się pod kursorem myszy przed zoomem;
- ten czas pozostaje pod tym samym pikselem po zoomie;
- zoom jest multiplikatywny i symetryczny; bazowy krok to około `1.15` na jednostkę wheel;
- dodatni wheel = zoom in;
- minimalny sensowny span X to jeden krok próbkowania `dt`;
- nie nakładamy sztucznego maksymalnego zoom-out.

`Fit X` ustawia `0..duration`.

Viewport może znaleźć się częściowo lub całkowicie poza datasetem. Jest to dozwolone. Dane, Values, cursory i markery zachowują własne reguły dotyczące granic datasetu.

## 11. Niezależny Y

Każda seria utrzymuje własne `viewMin/viewMax`.

Zmiana active nie resetuje Y. Powrót do serii przywraca jej wcześniejszy zakres.

### Fit Y

`Fit Y` działa tylko dla active visible series:

- używa globalnego `dataMin/dataMax` całej serii;
- nie zależy od aktualnego X;
- dodaje około 5% marginesu.

### Pan Y

- `Alt + drag` przesuwa Y active visible series;
- zachowanie jest typu „grab content” — przebieg podąża za ruchem myszy;
- X pozostaje nieruchomy;
- brak sztucznych granic zakresu Y.

### Zoom Y

- `Alt + wheel` skaluje `viewMin/viewMax` active visible series;
- punktem kotwiczenia jest środek bieżącego zakresu Y, nie pozycja kursora;
- używany jest ten sam prosty multiplikatywny krok około `1.15` na jednostkę wheel;
- zakres jest chroniony jedynie przed zerowym/niefinitym spanem.

Jeżeli nie ma active visible series, `Alt + drag` i `Alt + wheel` nie wykonują żadnej nawigacji i nie przechodzą do X.

## 12. Aktywna seria

Może istnieć zero lub jedna active series. Po Reset nie ma active.

Active visible series:

- jest rysowana na końcu;
- właściwa linia ma około `1.5 px` i pełny własny kolor;
- może mieć highlight rysowany pod nią;
- domyślny `Halo`: około `7 px`, kolor serii, alpha około `0.22`;
- `Outline`: około `5 px`, neutralny kolor tekstu, alpha około `0.65`;
- highlight i linia używają dokładnie tej samej geometrii screen-space;
- highlight nie rozszerza hit-testu.

Pozostałe widoczne serie mają linię około `1 px` i pełny kolor.

Jeżeli active series zostanie ukryta:

- logicznie nadal pozostaje active;
- nie istnieje active visible series;
- semantic Y i kolorowy grid znikają;
- Fit Y, Alt-pan i Alt-wheel są nieaktywne.

## 13. Wybór serii przez RMB

Plain RMB na plot area wykonuje hit-test widocznych serii.

Reguły:

- tolerancja około `5 px`;
- przy kilku kandydatach wygrywa najbliższa geometria w pikselach;
- RMB na nieaktywnej serii ustawia ją jako active;
- RMB na active series czyści active;
- RMB poza liniami czyści active.

Hit-test odpowiada rzeczywistej geometrii renderera, nie highlightowi.

Przy zoom-out algorytm przelicza lokalne około `5 px` wokół myszy na przedział czasu, wyznacza potrzebny zakres indeksów z `dt`, analizuje istotne segmenty i liczy point-to-segment distance w screen-space.

Na początku brak dodatkowego indeksu przestrzennego i LOD.

## 14. Semantic Y i grid Y

Widoczna semantic Y axis jest jedna, po lewej stronie, i dotyczy tylko active visible series.

Ticki Y:

- wynikają z `viewMin..viewMax` active series;
- używają klasycznej rodziny „nice numbers” `1, 2, 5 × 10^n`;
- wybierany krok ma dawać czytelne odstępy, docelowo około `60–80 px` między opisanymi liniami;
- ticki mogą wychodzić poza globalne `dataMin..dataMax`, jeżeli należą do aktualnego viewportu Y;
- format domyślnie używa 2 miejsc po przecinku;
- dla małego span można zwiększyć precyzję;
- dla bardzo dużych/małych liczb dopuszczalny jest format naukowy/skrócony.

Nazwa active series jest pokazana przy osi lub w górnej części plot area.

Layout osi ma stały budżet szerokości wystarczający dla eksperymentu; zmiana active lub wartości ticków nie może przesuwać plot area.

### Neutralny grid

Zawsze obecny i subtelny:

- pionowe major odpowiadają tickom X;
- neutralne poziome linie są czysto wizualnym podziałem plot area i nie reprezentują konkretnej serii;
- ich liczba wynika z wysokości plot area, z podobnym docelowym spacingiem około `60–80 px`.

### Kolorowy grid active

Gdy istnieje active visible series, dodatkowe poziome linie w jej kolorze odpowiadają semantic Y ticks i korzystają z jej `viewMin/viewMax`.

## 15. Widoczność i kolory

Ukryta seria:

- nie jest renderowana;
- nie uczestniczy w RMB hit-test;
- nie pojawia się w Values;
- zachowuje dane, kolor, active state i Y state.

Po ponownym pokazaniu wraca z poprzednim `viewMin/viewMax`.

Każda seria ma deterministyczny kolor. Nie używamy krótkiej palety, która powtarza barwy przy dużej liczbie serii.

## 16. Custom Legend

Custom Legend jest pływającym oknem ImGui.

- pokazuje wszystkie serie, także ukryte;
- steruje visibility;
- kliknięcie nazwy inactive series ustawia active;
- kliknięcie nazwy active series czyści active;
- ukryta seria może zostać active bez automatycznego pokazania;
- kolejność jest stała.

Nie ma drugiej legendy będącej częścią renderera wykresu.

## 17. Values i crosshair

`Values` jest osobnym pływającym oknem ImGui.

Pokazuje wszystkie widoczne serie w stałej kolejności. Active series pozostaje na swojej pozycji i jest wyróżniona pogrubionym tekstem. Przy każdej serii widoczny jest kolorowy znacznik.

Odczyt:

- działa tylko, gdy mysz jest wewnątrz plot area i czas pod myszą mieści się w datasetcie;
- po wyjściu z plot area jest czyszczony;
- przed pierwszą lub po ostatniej próbce brak odczytu;
- wartości są liniowo interpolowane pomiędzy sąsiednimi próbkami;
- hidden series nie są interpolowane ani wyświetlane.

Czas używa formatu z milisekundami, np. `mm:ss.mmm`, a po godzinie `hh:mm:ss.mmm`.

Crosshair:

- jest pionową linią czasu;
- podąża za dokładnym mouse X;
- nie snapuje do próbek;
- działa tylko nad plot area;
- może być ukryty niezależnie od Values.

## 18. Kursory A/B

Są dokładnie dwa kursory: A i B.

- przechowują czas, nie pozycję ekranową;
- po pan/zoom pozostają przy tym samym czasie;
- startują ukryte;
- mają różne kontrastowe kolory;
- linia około `2 px`;
- etykieta `A` / `B` przy górnej krawędzi;
- brak collision avoidance etykiet w pierwszej wersji.

Ustawianie nad plot area:

- `Shift + LMB` ustawia/przenosi A do czasu kliknięcia;
- `Ctrl + LMB` ustawia/przenosi B;
- jeśli czas kliknięcia jest poza datasetem, jest clampowany do `0..duration`.

Istniejący kursor przeciąga się plain LMB bez modyfikatora. Czas aktualizuje się w każdej klatce drag i jest clampowany do datasetu.

`Ctrl + RMB` na kursorze ukrywa go.

Render, hit-test i drag używają wspólnego systemu screen/data.

## 19. Markery

Marker jest pionową linią w konkretnym czasie.

- kilka markerów, nie setki;
- numeracja `0, 1, 2, ...`;
- usunięty numer nie wraca;
- Reset zeruje licznik;
- wspólny kolor inny niż A/B;
- linia około `1 px`;
- zawsze widoczna etykieta numeru;
- brak selected state i collision avoidance etykiet w pierwszej wersji.

Dodawanie:

- `Alt + LMB` rozpoczyna kandydat na marker;
- jeżeli do puszczenia LMB ruch pozostaje poniżej progu około `4–5 px`, powstaje marker;
- marker nie snapuje do próbki;
- czas jest clampowany do datasetu.

Jeżeli ruch przekroczy próg:

- przy active visible series gest staje się pan Y;
- bez active visible series kandydat markera jest anulowany i nic więcej się nie dzieje.

Przeciąganie markera: plain LMB. Usuwanie: `Ctrl + RMB`.

## 20. Priorytety i własność inputu

Gesty wykresu zaczynają się tylko wtedy, gdy ich początkowy press/wheel wystąpi nad plot area. Po rozpoczęciu drag właściciel gestu zachowuje go do release, nawet jeśli kursor wyjdzie poza plot area.

Modifier gestures są wyłączne. Gest z `Shift`, `Ctrl` lub `Alt` nie przechodzi awaryjnie do zwykłego pan X ani RMB selection.

Docelowa kolejność:

1. `Shift + LMB` — set A.
2. `Ctrl + LMB` — set B.
3. `Alt + LMB` — marker candidate; po threshold ewentualnie pan Y.
4. `Alt + wheel` — zoom Y active visible series; bez active visible series ignorowany.
5. plain LMB na cursor/marker — drag elementu.
6. plain LMB poza interaktywnym overlayem — pan X.
7. plain wheel — zoom X względem mouse X.
8. `Ctrl + RMB` na cursor/marker — hide/remove jednego elementu.
9. `Ctrl + RMB` bez trafienia w cursor/marker — ignorowany; nie przechodzi do selection.
10. plain RMB — hit-test serii i toggle/clear active.

Jeżeli cursor i marker leżą na sobie, wystarczy możliwość złapania/usunięcia jednego z nich; nie wymagamy deterministycznego wyboru typu.

Input routing jest całkowicie app-owned.

## 21. Render order

Docelowa kolejność logiczna:

1. tło plot area;
2. neutralny grid;
3. nieaktywne widoczne serie;
4. highlight active series;
5. właściwa linia active series;
6. kolorowy grid/semantic layer w miejscu zapewniającym najlepszą czytelność;
7. cursory, markery, crosshair i ich etykiety;
8. dekoracje osi i tekst ponad plot area.

Położenie kolorowego grida względem linii może zostać skorygowane po manualnej ocenie. Nie zmienia to semantyki danych ani inputu.

## 22. Rendering i clipping

Pierwsza implementacja jest możliwie prosta:

- bez LOD;
- bez pełnej drugiej kopii geometrii;
- wartości są transformowane do screen-space podczas renderowania;
- clipping do plot area;
- przetwarzanie można ograniczać do indeksów obejmujących widoczny X plus mały zapas na krawędziach;
- active series jest rysowana na końcu.

Optymalizacje wprowadzamy dopiero po pomiarach Reference i Stress Raw.

## 23. Stabilność i przypadki graniczne

- wszystkie transformacje muszą obsługiwać resize plot area;
- zerowa lub ujemna szerokość/wysokość plot area oznacza brak renderowania i inputu w tej klatce;
- niepoprawny/niefinity viewport jest odzyskiwany przez Fit odpowiedniej osi;
- X może być poza datasetem, ale Values nie pokazuje wtedy danych;
- cursory i markery zawsze pozostają clampowane do datasetu;
- hidden active zachowuje active state, ale nie ma narzędzi Y;
- scroll nad innymi oknami ImGui nie jest przejmowany przez wykres;
- dokładne stałe wizualne i szybkość zoomu mogą być później lekko dostrojone po manualnym teście bez zmiany architektury, o ile zachowana zostaje opisana semantyka.

## 24. Weryfikacja

Projekt ma trzy warstwy:

1. **functional C++** — logika, transformacje, tick generation, stan, matematyka i edge-case'y;
2. **GUI integration** — rzeczywisty routing inputu i stan widgetów Dear ImGui;
3. **manual** — wygląd, czytelność, ergonomia, płynność i subiektywne zachowanie.

Automaty mają chronić semantykę, nie wymuszać sztucznej architektury. Małe funkcje/model wydzielamy tylko wtedy, gdy są sensowne także dla kodu produkcyjnego.

Minimalnie należy automatycznie pokryć:

- data/screen i screen/data transforms;
- Fit X/Y;
- pan/zoom X;
- pan/zoom Y;
- tick generation X/Y;
- active/visibility;
- hit-test segmentów;
- Values/interpolation;
- cursory;
- markery;
- input priority.

## 25. Kryterium sukcesu

Po zakończeniu eksperymentu musimy móc ocenić:

- czy niezależne Y jest naturalne w własnym rendererze;
- czy osie i layout są stabilne i proste w utrzymaniu;
- czy jeden system data/screen upraszcza rendering, Values, hit-test, cursory i markery;
- czy routing inputu jest przewidywalny;
- czy Reference dataset jest praktycznie używalny bez LOD;
- gdzie zaczyna się realny problem wydajności;
- ile własnego kodu wymaga rozwiązanie;
- czy dodatkowa kontrola uzasadnia koszt utrzymania.

Wynik może być pozytywny, negatywny lub mieszany. Prototyp ma dostarczyć danych do późniejszej decyzji o warstwie wykresów LogViewera.

## 26. Status specyfikacji

Ta wersja jest bazową, kompletną specyfikacją pierwszego eksperymentu custom chart.

Podczas implementacji można doprecyzować szczegóły techniczne lub parametry wizualne, ale zmiana opisanej semantyki użytkowej, modelu danych, mapy gestów albo zakresu eksperymentu wymaga jawnej aktualizacji tego dokumentu i `DECISIONS.md`.