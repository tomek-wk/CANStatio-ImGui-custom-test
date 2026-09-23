# CANStatio ImPlot Test — instrukcje dla AI

## Cel repozytorium

To repozytorium jest samodzielnym eksperymentem służącym do oceny Dear ImGui + ImPlot jako potencjalnego frontendu wykresu dla CANstatio LogViewer.

Nie jest to aktualna implementacja produkcyjnego LogViewera i nie należy automatycznie synchronizować jego architektury ani UX z głównym repozytorium CANStatio.

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
2. `docs/AUTOMATED_TESTS.md` — wyłącznie w zakresie automatycznych testów, ich narzędzi i uruchamiania;
3. `docs/PROJECT_SPEC.md`;
4. `docs/DECISIONS.md`;
5. `docs/PROGRESS.md`;
6. bieżąca implementacja.

Jeżeli implementacja i dokumentacja się rozjechały, nie dopasowuj po cichu wymagań do kodu. Ustal, która warstwa jest nieaktualna i popraw ją jawnie.

## Model pracy z branchami

Eksperyment infrastrukturalny `feature/imgui-test-engine` został zakończony i zintegrowany do `main` przez PR #1 2026-09-23.

Od tego momentu **normalny rozwój aplikacji odbywa się liniowo na `main`**. Nie twórz osobnego brancha dla każdej zwykłej funkcji. Osobny branch ma sens dopiero przy zmianie, która rzeczywiście korzysta z izolacji, np. dużym eksperymencie architektonicznym, zmianie renderera, modelu danych, toolchainu albo infrastruktury pracy.

Jeżeli dla przyszłego specjalnego brancha zostanie zapisana osobna merge policy, należy jej bezwzględnie przestrzegać. Historyczna zasada D-043 dotyczyła `feature/imgui-test-engine` i została spełniona jawną zgodą użytkownika przed PR #1.

## Zasady pracy

Przy każdym większym zadaniu:

1. sprawdź aktualny stan repozytorium i właściwy branch;
2. ustal minimalny zakres zmian;
3. nie dodawaj abstrakcji, klas ani zależności bez realnej potrzeby;
4. nie refaktoruj produkcyjnego kodu tylko po to, aby pasował do schematu testów;
5. po zmianach uruchom build, jeśli środowisko na to pozwala;
6. uruchom odpowiednią warstwę testów: functional, GUI albo obie;
7. nie deklaruj zachowania jako zweryfikowanego, jeśli nie zostało rzeczywiście uruchomione;
8. wygląd, ergonomię i UX traktuj oddzielnie od automatycznego PASS/FAIL;
9. po istotnej zmianie zaktualizuj `docs/PROGRESS.md`;
10. jeśli zapadła nowa istotna decyzja, dopisz ją do `docs/DECISIONS.md`, a jeśli zmienia wymaganie — również do `docs/PROJECT_SPEC.md`.

## Granice eksperymentu

Na obecnym etapie nie dodawaj bez osobnej decyzji:

- parsera CSV ani otwierania plików;
- DataCore ani modelu projektu CANStatio;
- Overview/minimapy;
- LOD/downsamplingu, dopóki surowe renderowanie nie pokaże realnej potrzeby;
- wielu osi Y ImPlot jako substytutu niezależnego Y każdej serii;
- `implot_internal.h`;
- integracji z głównym repo CANStatio;
- zewnętrznego OS-level frameworka automatyzacji GUI, jeśli Dear ImGui Test Engine wystarcza;
- nowego frameworka testów funkcyjnych, jeśli GoogleTest + CTest pokrywa potrzeby;
- dodatkowych warstw emulacji/renderingu tylko po to, aby upodobnić hostowany runner do docelowego Windows PC.

Automatyzacja ma pokrywać powtarzalne regresje funkcjonalne. Ręczne testy nadal są wymagane dla wyglądu, ergonomii i UX.

## Bazowy stack

- C++20
- Windows jako pierwsza platforma
- GCC / MinGW-w64 (MSYS2 UCRT64)
- CMake + Ninja
- GLFW
- OpenGL 3.3 Core Profile
- Dear ImGui
- ImPlot

Przypięte biblioteki aplikacji są pobierane przez CMake `FetchContent`:

- Dear ImGui `v1.92.9b`
- ImPlot `v1.0`
- GLFW `3.5.1`
- Dear ImGui Test Engine commit `508a8fc8dacac2f346d353fed31b9bc90ed29adc`

Testy funkcyjne używają GoogleTest znalezionego przez `find_package(GTest REQUIRED)`; nie jest on pobierany przez `FetchContent`.

## Strategia testów

Projekt ma trzy poziomy weryfikacji:

1. **functional C++** — GoogleTest + CTest dla logiki, matematyki, stanu i edge-case'ów bez GUI;
2. **GUI integration** — Dear ImGui Test Engine dla rzeczywistych interakcji ImGui/ImPlot i input routing;
3. **manual** — wygląd, czytelność, ergonomia i subiektywna płynność.

Nie kopiuj architektury testów z głównego CANstatio. Nie twórz z góry biblioteki `core` ani dużych warstw tylko dla testów.

### Wybór środowiska testowego

**Docelowy Windows PC / Test Agent jest podstawową i normalną ścieżką testową.** Gdy jest dostępny, używaj go do configure/build, functional i GUI. Aktualny zweryfikowany zestaw to `21/21 functional` oraz `19/19 GUI`.

**GitHub-hosted Windows** (`.github/workflows/windows-ci.yml`) jest wyłącznie ręcznym fallbackiem, gdy target PC/Test Agent jest niedostępny. Workflow wykonuje pełny configure/build w MSYS2 UCRT64 oraz functional CTest. Nie używaj go jako zwykłego checkpointu, dodatkowego obowiązkowego CI ani przy każdym commicie.

Workflow jest `workflow_dispatch` only. Standardowy GitHub-hosted Windows runner nie wykonuje GUI suite z powodu braku użytecznego WGL/OpenGL w zweryfikowanym środowisku.

Eksperymentalny Linux GitHub Actions nie jest częścią bieżącego systemu testowego i jego workflow został usunięty. Nie przywracaj go bez nowej jawnej decyzji użytkownika.

## Aktualny etap funkcjonalny

Stage 6A — kursory A/B — jest zaimplementowany, automatycznie zweryfikowany i ręcznie zaakceptowany.

- `Shift + LMB` ustawia/przenosi A;
- `Ctrl + LMB` ustawia/przenosi B;
- zwykły LMB przeciąga istniejący kursor;
- `Ctrl + RMB` ukrywa trafiony kursor;
- czasy są clampowane do datasetu;
- kursor zachowuje czas przy nawigacji X;
- `Alt + drag` jest normalnym pan Y aktywnej serii i nie ma specjalnej semantyki związanej z kursorem;
- stan jest przechowywany w małym produkcyjnym `CursorModel`, a overlay używa publicznego `ImPlot::DragLineX`.

Przed Stage 6B wykonano też cleanup starych problemów wizualnych/layoutu:

- semantic Y ma stale rezerwowany app-owned gutter, dzięki czemu zmiana active nie zmienia `plotPos`/`plotSize`;
- Halo/Outline jest rysowane jako screen-space `ImDrawList::AddPolyline`, a właściwa linia active nadal pozostaje cienką linią ImPlot.

Oba rozwiązania przeszły automatyczną regresję tam, gdzie jest to mierzalne, oraz ręczną ocenę wizualną użytkownika.

Następna część Stage 6 to **Stage 6B — markery i `Alt-click` vs `Alt-drag`**.

## Najważniejsze założenia techniczne

- wszystkie serie mają wspólny regularny X;
- każda seria posiada niezależny stan Y;
- fizyczna oś Y ImPlot jest wspólną przestrzenią wewnętrzną `0..1`;
- semantyczne dekoracje Y są app-owned i mają stały gutter;
- dane każdej serii są mapowane do przestrzeni `0..1` na podstawie jej własnego `viewMin/viewMax`;
- tylko aktywna widoczna seria prezentuje semantyczną oś Y;
- standardowy pan/zoom Y ImPlot jest wyłączony, a Y sterowane przez warstwę aplikacji;
- X pozostaje natywnie nawigowany przez ImPlot, zewnętrzny stan `xMin/xMax` jest synchronizowany;
- ukrytej serii nie przekazujemy do renderera;
- aktywna seria jest rysowana na końcu;
- publiczne API ImPlot/ImGui jest preferowane.

Pełna semantyka znajduje się w `docs/PROJECT_SPEC.md`.

## Aktualizacja dokumentacji

`docs/PROGRESS.md` powinien zawsze rozróżniać:

- co istnieje;
- co zostało automatycznie zweryfikowane;
- czy weryfikacja pochodzi z target Windows/Test Agent czy awaryjnego GitHub Windows;
- co zostało ręcznie zweryfikowane;
- czego jeszcze nie sprawdzono;
- najbliższy krok.
