# CANStatio ImGui Custom Chart Test — instrukcje dla AI

## Cel repozytorium

To repozytorium jest samodzielnym eksperymentem służącym do oceny Dear ImGui z własną implementacją wykresu jako potencjalnego frontendu wykresów dla CANstatio LogViewer.

Nie jest to produkcyjny LogViewer. Nie należy automatycznie synchronizować jego architektury ani UX z głównym repozytorium CANStatio.

## Nadrzędna granica repozytorium

Pracuj wyłącznie w `CANStatio-ImGui-custom-test`.

Nie zapisuj, nie zmieniaj i nie usuwaj niczego w innych repozytoriach. Repozytorium Test Agenta może być używane wyłącznie jako kanał komunikacji ze środowiskiem testowym; jego zawartość nie jest częścią zakresu zmian.

## Co przeczytać przed pracą

Przed istotną zmianą przeczytaj w tej kolejności:

1. `AGENTS.md`
2. `docs/PROJECT_SPEC.md`
3. `docs/DECISIONS.md`
4. `docs/PROGRESS.md`
5. `docs/AUTOMATED_TESTS.md` — jeśli zadanie dotyczy testów lub weryfikacji
6. odpowiednie pliki źródłowe

## Hierarchia źródeł prawdy

W przypadku sprzeczności obowiązuje kolejność:

1. bieżące jawne polecenie użytkownika;
2. `docs/AUTOMATED_TESTS.md` — wyłącznie w zakresie testów, ich narzędzi i uruchamiania;
3. `docs/PROJECT_SPEC.md`;
4. `docs/DECISIONS.md`;
5. `docs/PROGRESS.md`;
6. bieżąca implementacja.

Skopiowany kod startowy może zawierać rozwiązania należące do poprzedniej architektury. Nie traktuj ich jako wymagania tylko dlatego, że istnieją w kodzie.

## Model pracy z branchami

Normalny rozwój odbywa się liniowo na `main`.

Nie twórz brancha dla każdej zwykłej funkcji. Osobny branch ma sens dopiero dla zmiany, która rzeczywiście korzysta z izolacji, np. dużego eksperymentu architektonicznego, zmiany modelu danych, toolchainu albo infrastruktury testowej.

## Zasady pracy

Przy większym zadaniu:

1. sprawdź aktualny stan `CANStatio-ImGui-custom-test` i właściwy branch;
2. ustal minimalny zakres zmian;
3. najpierw utrzymuj zgodność z bieżącą specyfikacją, nie ze skopiowanym rozwiązaniem technicznym;
4. nie dodawaj abstrakcji, klas ani zależności bez realnej potrzeby;
5. nie twórz warstw tylko po to, aby zachować strukturę starego renderera;
6. po zmianach uruchom build, jeśli środowisko na to pozwala;
7. uruchom odpowiednią warstwę testów: functional, GUI albo obie;
8. nie deklaruj zachowania jako zweryfikowanego, jeśli nie zostało rzeczywiście uruchomione;
9. wygląd, ergonomię i UX traktuj oddzielnie od automatycznego PASS/FAIL;
10. po istotnej zmianie zaktualizuj `docs/PROGRESS.md`;
11. jeśli zapadła nowa istotna decyzja, dopisz ją do `docs/DECISIONS.md`, a jeśli zmienia wymaganie — również do `docs/PROJECT_SPEC.md`.

## Granice eksperymentu

Na obecnym etapie nie dodawaj bez osobnej decyzji:

- parsera CSV ani otwierania plików;
- DataCore ani modelu projektu CANStatio;
- Overview/minimapy;
- irregular sampling ani osobnych timestampów serii;
- LOD/downsamplingu, dopóki raw rendering nie pokaże realnej potrzeby;
- integracji z głównym repo CANStatio;
- dodatkowej biblioteki wykresowej;
- zewnętrznego OS-level frameworka automatyzacji GUI, jeśli Dear ImGui Test Engine wystarcza;
- nowego frameworka testów funkcyjnych, jeśli GoogleTest + CTest pokrywa potrzeby.

## Bazowy stack

- C++20
- Windows jako pierwsza platforma
- GCC / MinGW-w64 (MSYS2 UCRT64)
- CMake + Ninja
- GLFW
- OpenGL 3.3 Core Profile
- Dear ImGui
- Dear ImGui Test Engine
- GoogleTest + CTest

Przypięte biblioteki:

- Dear ImGui `v1.92.9b`
- GLFW `3.5.1`
- Dear ImGui Test Engine commit `508a8fc8dacac2f346d353fed31b9bc90ed29adc`

## Docelowa odpowiedzialność własnego wykresu

Warstwa aplikacji ma być właścicielem:

- plot area i jego stabilnego layoutu;
- transformacji czasu na X w pikselach i odwrotnie;
- transformacji wartości każdej serii na Y w pikselach i odwrotnie;
- osi, ticków, etykiet i grida;
- renderowania serii i clippingu;
- hit-testu serii i overlayów;
- routingu inputu;
- pan/zoom X;
- pan/zoom Y aktywnej serii;
- crosshair;
- kursorów A/B;
- markerów.

Nie zachowuj modelu wspólnej sztucznej przestrzeni Y tylko dlatego, że był używany w skopiowanej implementacji. Każda seria ma własny zakres `viewMin/viewMax`, a renderer może bezpośrednio mapować surową wartość do pikseli plot area.

## Najważniejsze założenia funkcjonalne

- wszystkie serie mają wspólny regularny X;
- każda seria ma niezależny stan Y;
- plot area nie może zmieniać geometrii tylko dlatego, że zmieniła się aktywna seria;
- tylko aktywna widoczna seria prezentuje semantyczne wartości Y;
- ukryta seria zachowuje dane, kolor, active state i Y state;
- aktywna seria jest rysowana na końcu;
- X i Y navigation są obsługiwane przez aplikację;
- Custom Legend jest jedyną docelową legendą funkcjonalną;
- Values, crosshair, kursory i markery korzystają ze wspólnego systemu współrzędnych własnego wykresu.

Pełna semantyka znajduje się w `docs/PROJECT_SPEC.md`.

## Strategia testów

Projekt ma trzy poziomy weryfikacji:

1. **functional C++** — logika, transformacje, stan, matematyka i edge-case'y bez GUI;
2. **GUI integration** — rzeczywiste interakcje Dear ImGui i input routing przez Dear ImGui Test Engine;
3. **manual** — wygląd, czytelność, ergonomia i subiektywna płynność.

Docelowy Windows PC / Test Agent jest podstawową ścieżką testową. GitHub-hosted Windows pozostaje wyłącznie ręcznym fallbackiem, gdy target PC/Test Agent jest niedostępny.

## Stan przejściowy

Repozytorium zawiera skopiowaną działającą bazę, która ma pomóc zachować sprawdzone zachowanie użytkowe i istniejące małe modele danych/stanu.

Jej renderer, input routing, osie i overlaye są materiałem przejściowym. Wraz z migracją należy usuwać elementy, które nie mają sensu w architekturze własnego wykresu, zamiast odtwarzać je mechanicznie.

`docs/PROGRESS.md` musi zawsze rozróżniać:

- co działa jeszcze tylko w bazie przejściowej;
- co zostało już przeniesione do własnego wykresu;
- co zostało automatycznie zweryfikowane po migracji;
- co zostało ręcznie zweryfikowane;
- czego jeszcze nie sprawdzono;
- najbliższy krok.