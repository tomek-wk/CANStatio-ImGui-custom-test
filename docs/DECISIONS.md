# Rejestr decyzji

Ten plik zapisuje istotne decyzje eksperymentu. Ma pomóc wrócić do projektu z nowego kontekstu bez ponownego rozstrzygania tych samych tematów.

Jeżeli decyzja zostanie zmieniona, nie usuwaj starej bez śladu. Oznacz ją jako zastąpioną i dopisz nową decyzję z datą.

## 2026-09-22

### D-001 — osobne repozytorium

**Decyzja:** eksperyment jest prowadzony w osobnym repo `CANStatio-ImPlot-Test`, a nie jako katalog w głównym CANstatio.

**Powód:** test może swobodnie zmieniać zależności, strukturę i UX bez ryzyka wpływu na właściwy LogViewer.

### D-002 — FetchContent zamiast ręcznego `third_party`

**Decyzja:** Dear ImGui, ImPlot i GLFW są pobierane automatycznie przez CMake `FetchContent`.

**Powód:** prostszy start, reprodukowalne wersje i brak ręcznego kopiowania bibliotek.

**Konsekwencja:** pierwsza konfiguracja CMake wymaga internetu. Kolejne buildy mogą korzystać z już pobranych źródeł w katalogu build.

### D-003 — przypięte wersje bibliotek

**Decyzja:** początkowa baza eksperymentu to:

- Dear ImGui `v1.92.9b`;
- ImPlot `v1.0`;
- GLFW `3.5.1`.

**Powód:** test ma być powtarzalny. Aktualizacja zależności nie powinna następować przypadkiem.

### D-004 — GLFW + OpenGL 3.3 Core

**Decyzja:** backend okna/renderera to GLFW + OpenGL 3.3 Core Profile.

**Powód:** mały boilerplate, przenośność oraz wystarczająca kompatybilność z docelowym sprzętem testowym Intel HD Graphics 530.

### D-005 — test wykresu, nie całego LogViewera

**Decyzja:** brak parsera, plików, DataCore, modelu projektu i innych funkcji aplikacji.

**Powód:** chcemy odizolować pytanie o jakość i koszt warstwy wykresu ImPlot.

### D-006 — brak Overview w pierwszym eksperymencie

**Decyzja:** Overview/minimapa pozostaje poza zakresem.

**Powód:** jest osobnym problemem i nie jest potrzebna do oceny podstawowego modelu wykresu.

### D-007 — regularny wspólny X

**Decyzja:** wszystkie serie w teście mają wspólne regularne timestampy: `x = index * dt`.

**Powód:** irregular sampling i osobne osie czasu nie są potrzebne do odpowiedzi na główne pytania eksperymentu.

### D-008 — dane Y jako `float`

**Decyzja:** pierwsza wersja używa tylko ciągłych serii `float`.

**Powód:** step/digital i inne typy danych zostają na później.

### D-009 — deterministic generator

**Decyzja:** każdy preset ma stały seed i po ponownym wygenerowaniu daje te same dane.

**Powód:** łatwiejsze ręczne porównywanie zachowania i wydajności.

### D-010 — raw data first

**Decyzja:** pierwsza implementacja nie ma LOD/downsamplingu.

**Powód:** najpierw potrzebny jest prosty punkt odniesienia i rzeczywisty pomiar zachowania ImPlot.

**Warunek rewizji:** jeśli Reference lub Stress Raw pokaże realny problem, można dodać porównawczy wariant LOD.

### D-011 — niezależny stan Y każdej serii

**Decyzja:** każda seria przechowuje własne `viewMin/viewMax`.

**Powód:** pan/zoom jednej serii nie może zmieniać pozostałych, a powrót do serii ma odtwarzać jej wcześniejszy stan.

### D-012 — fizyczna oś ImPlot Y = `0..1`

**Decyzja:** wszystkie serie są renderowane we wspólnej wewnętrznej przestrzeni Y `0..1`, a surowe wartości każdej serii są transformowane według jej `viewMin/viewMax`.

**Powód:** pozwala wykorzystać jeden fizyczny plot area i zachować niezależne Y bez klasycznych wielu osi Y.

### D-013 — transformacja podczas renderowania

**Decyzja:** pierwszy wariant nie utrzymuje pełnego cache'a znormalizowanych wartości. Transformacja ma następować podczas przekazywania danych do ImPlot przez publiczną ścieżkę getter/callback lub jej odpowiednik w używanej wersji.

**Powód:** minimalny kod i pojedyncze źródło danych.

**Warunek rewizji:** koszt można porównać z cache'em, jeśli pomiar wykaże problem.

### D-014 — Fit Y z całej serii

**Decyzja:** Fit Y aktywnej serii używa globalnego `dataMin/dataMax`, niezależnie od aktualnego viewportu X, z około 5% marginesu.

**Powód:** stabilny i prosty punkt odniesienia dla eksperymentu.

### D-015 — własne sterowanie Y

**Decyzja:** standardowy pan/zoom Y ImPlot ma być wyłączony. `Alt + drag` przesuwa active Y, `Alt + wheel` skaluje active Y.

**Powód:** ImPlot nie może samodzielnie zmieniać wspólnej fizycznej osi `0..1`, bo semantyczny Y jest własnością poszczególnych serii.

### D-016 — natywny pan/zoom X pozostaje

**Decyzja:** zwykły LMB drag i wheel wykorzystują natywną nawigację X ImPlot, a aplikacja synchronizuje z nią `xMin/xMax`.

**Powód:** chcemy korzystać z ImPlot tam, gdzie odpowiada naszemu modelowi, zamiast przepisywać wszystko.

### D-017 — własne ticki czasu

**Decyzja:** ticki X generuje aplikacja na podstawie zoomu, szerokości i „nice” interwałów czasu.

**Powód:** potrzebujemy kontrolowanego formatu czasu względnego i komfortowego zagęszczenia etykiet.

### D-018 — jedna semantyczna oś Y aktywnej serii

**Decyzja:** tylko active visible series pokazuje opis wartości Y. Nie pokazujemy wielu klasycznych osi Y.

### D-019 — neutralny grid + kolorowy grid active

**Decyzja:** zawsze istnieje szary neutralny grid. Active visible series dodaje tylko poziomy grid w swoim kolorze.

Kolorowy grid jest pozycjonowany według bieżącego `viewMin/viewMax`, ale jego semantyczne linie mają występować tylko w globalnym zakresie `dataMin..dataMax` serii po transformacji.

Oś Y nadal opisuje cały bieżący `viewMin..viewMax`.

### D-020 — active series przez RMB hit-test

**Decyzja:** RMB w pobliżu przetransformowanej geometrii serii przełącza active; tolerancja około 5 px; najbliższa seria wygrywa.

RMB poza liniami czyści active. RMB na już aktywnej serii również czyści active.

**Uwaga:** to jest celowo eksperymentalny UX i nie oznacza automatycznej zmiany specyfikacji głównego CANstatio.

### D-021 — hit-test analizuje segmenty, nie tylko nearest sample

**Decyzja:** przy dużym zoom-out sprawdzane są segmenty należące do małego przedziału X odpowiadającego około 5 px wokół myszy.

**Powód:** krótki spike lub przejście musi pozostać klikalne, nawet gdy wiele próbek mieści się w jednym pikselu.

### D-022 — ukryta seria nie jest renderowana

**Decyzja:** hidden series jest całkowicie pomijana przy rysowaniu i hit-teście.

Jej dane, Y state, kolor i logiczny active state pozostają zachowane.

### D-023 — hidden active pozostaje logicznie active

**Decyzja:** ukrycie aktywnej serii nie czyści flagi active, ale powoduje brak active **visible** series. Oś Y i narzędzia Y są wtedy nieaktywne.

### D-024 — native legend tylko informacyjna

**Decyzja:** Native ImPlot Legend nie ma sterować visibility ani active. Hide/show buttons mają być wyłączone.

**Powód:** unikamy synchronizacji wewnętrznego stanu ImPlot i używania `implot_internal.h`.

### D-025 — Custom Legend jest funkcjonalna

**Decyzja:** Custom Legend pokazuje wszystkie serie, steruje visibility i pozwala przełączać active kliknięciem nazwy.

### D-026 — bez `implot_internal.h`

**Decyzja:** preferujemy wyłącznie publiczne API ImPlot.

**Warunek rewizji:** użycie internal API wymaga świadomej decyzji po stwierdzeniu, że publiczne API nie pozwala sensownie wykonać ważnej funkcji.

### D-027 — Values używa interpolacji liniowej

**Decyzja:** wartość pod dokładnym mouse X jest interpolowana liniowo pomiędzy sąsiednimi regularnymi próbkami.

Crosshair jest niezależnie przełączalną pionową linią i nie snapuje do próbek.

### D-028 — dokładnie dwa kursory A/B

**Decyzja:** A i B są czasami w datasetcie, mają własne kolory, można je ustawiać modifier+LMB, przeciągać i usuwać/hide przez Ctrl+RMB.

### D-029 — markery numerowane od 0

**Decyzja:** marker labels to `0, 1, 2, ...`; usunięty numer nie jest używany ponownie; Reset zeruje licznik.

**Uwaga:** to jest semantyka tego eksperymentu i nie musi odpowiadać obecnemu nazewnictwu markerów w głównym CANstatio.

### D-030 — Alt click vs Alt drag przez threshold

**Decyzja:** ruch poniżej około 4–5 px traktujemy jako Alt-click i dodanie markera; po przekroczeniu progu operacja staje się pan Y i marker nie powstaje.

### D-031 — kursory/markery nad seriami

**Decyzja:** interaktywne pionowe overlaye są rysowane nad przebiegami. Przy kolizji dwóch takich elementów wystarczy przejęcie jednego; nie wymagamy złożonego rozstrzygania overlapu w pierwszej wersji.

### D-032 — brak box select

**Decyzja:** box select nie jest implementowany w pierwszej wersji.

**Powód:** RMB jest potrzebny do eksperymentalnego wyboru serii, a box select nie jest potrzebny do głównego celu testu.

### D-033 — manualne testowanie

**Decyzja:** ten eksperyment nie zaczyna od formalnego frameworka automatycznych testów GUI ani progu FPS.

**Powód:** najpierw potrzebujemy szybko zweryfikować ergonomię, zakres custom code i realne zachowanie ImPlot.

### D-034 — dokumentacja jako kontekst do wznowienia pracy

**Decyzja:** `AGENTS.md`, `docs/PROJECT_SPEC.md`, `docs/DECISIONS.md` i `docs/PROGRESS.md` są utrzymywane razem z kodem.

**Powód:** nowy kontekst AI ma móc wznowić projekt bez odtwarzania historii rozmowy.

### D-035 — aktywna seria ma linię 3 px

**Decyzja:** grubość aktywnej widocznej serii zostaje zwiększona z 2 px do **3 px**. Pozostałe widoczne serie pozostają 1 px.

**Powód:** po pierwszym ręcznym teście użytkownik uznał wyróżnienie 2 px za zbyt mało czytelne.

**Zastępuje:** wcześniejsze założenie 2 px dla aktywnej serii.

### D-036 — modifier gestures przejmują input ImPlot przez publiczne `OverrideMod`

**Decyzja:** przed `BeginPlot` `ImPlotInputMap::OverrideMod` jest ustawiany na aktualnie trzymaną kombinację modyfikatorów. Dzięki temu gesty z Alt/Shift/Ctrl nie uruchamiają równocześnie natywnego inputu ImPlot, natomiast bez modyfikatorów ImPlot zachowuje natywny LMB pan X i wheel zoom X.

**Powód:** pozwala utrzymać publiczne API i rozdzielić własne narzędzia od natywnej nawigacji X bez `implot_internal.h`.

**Do weryfikacji:** praktyczne zachowanie przy przyszłych kursorach, markerach i kombinacjach modifierów.

### D-037 — aktywna seria ma linię 4 px

**Decyzja:** grubość aktywnej widocznej serii zostaje zwiększona z 3 px do **4 px**. Pozostałe widoczne serie pozostają 1 px.

**Powód:** po kolejnym ręcznym teście użytkownik uznał 3 px nadal za zbyt słabe wyróżnienie.

**Zastępuje:** D-035 oraz wcześniejsze założenie 2 px.

**Status:** zastąpiona przez D-038.

### D-038 — cienka linia active + przełączalne Halo / Outline

**Decyzja:** aktywna widoczna seria nie jest już wyróżniana przez samo pogrubienie właściwej linii danych. Właściwy przebieg active ma **1.5 px**, a pod nim rysowana jest osobna warstwa wyróżnienia wybierana w `Test Controls`:

- `Halo` — **7 px**, kolor aktywnej serii, alpha około **0.22**;
- `Outline` — **5 px**, neutralny kolor tekstu bieżącego stylu i alpha około **0.65**.

Domyślnym trybem jest `Halo`. Pozostałe widoczne serie pozostają 1 px.

Warstwa wyróżnienia jest osobnym przebiegiem ImPlot rysowanym przed cienką linią active i używa publicznych `ImPlotItemFlags_NoLegend | ImPlotItemFlags_NoFit`, więc nie tworzy dodatkowej pozycji w native legend i nie wpływa na Fit. Hit-test RMB nadal działa na rzeczywistej geometrii danych i nie jest rozszerzany przez szerokość Halo/Outline.

**Powód:** dalsze pogrubianie właściwej linii pogarsza precyzję wizualnego odczytu położenia przebiegu. Oddzielenie cienkiego rdzenia od szerokiego wyróżnienia pozwala zachować dokładną geometrię i jednocześnie mocno wskazać active.

**Zastępuje:** D-037, D-035 oraz wcześniejsze założenie 2 px dla active.

**Do weryfikacji:** porównać Halo i Outline ręcznie, szczególnie na przecięciach serii, oraz ocenić czy szerokość/alpha wymagają korekty.

## 2026-09-23

### D-039 — automatyczne testy GUI przez Dear ImGui Test Engine

**Decyzja:** do eksperymentu zostaje wprowadzony Dear ImGui Test Engine do automatycznych testów powtarzalnego zachowania GUI i regresji funkcjonalnych.

**Powód:** po ręcznym zweryfikowaniu podstawowych etapów projekt ma już wystarczająco stabilne zachowanie, aby automatyzacja zmniejszała koszt regresji przy dalszej rozbudowie interakcji.

**Zastępuje w zakresie dalszego rozwoju:** D-033 jako założenie o braku formalnego frameworka GUI. D-033 pozostaje historycznie prawdziwe dla początkowych etapów eksperymentu.

### D-040 — automatyzacja nie zastępuje ręcznej oceny UX

**Decyzja:** testy automatyczne obejmują logikę, stan i deterministyczne interakcje. Wygląd, czytelność, ergonomia i subiektywna płynność nadal wymagają ręcznej weryfikacji.

**Powód:** Test Engine dobrze wykrywa regresje zachowania, ale nie powinien podejmować za użytkownika decyzji o jakości wizualnej eksperymentu.

### D-041 — przypięta wersja Dear ImGui Test Engine

**Decyzja:** początkowa integracja używa commita `508a8fc8dacac2f346d353fed31b9bc90ed29adc` z repo `ocornut/imgui_test_engine`.

**Powód:** projekt używa Dear ImGui `v1.92.9b`; wybrany commit Test Engine pochodzi z linii sprzed późniejszych zmian kompatybilności dla ImGui 1.93. Test Engine i Dear ImGui należy aktualizować świadomie razem, a nie przez ruchomy branch `main`.

### D-042 — osobny branch automatyzacji

**Decyzja:** wdrożenie jest prowadzone na branchu `feature/imgui-test-engine`. `main` pozostaje bazą bez zmian do czasu zakończenia i weryfikacji prac.

### D-043 — merge do `main` wyłącznie po jawnej zgodzie użytkownika

**Decyzja:** branch `feature/imgui-test-engine` nie może zostać zmergowany do `main` bez jawnej zgody użytkownika odnoszącej się do merge.

**Konsekwencja:** poprawny build, wszystkie testy PASS, gotowy PR, brak konfliktów ani ogólne polecenie kontynuowania pracy nie stanowią zgody na merge.

### D-044 — pierwszy poziom automatyzacji to smoke + widget interactions

**Decyzja:** pierwsze wdrożenie obejmuje runner `--run-tests`, opcjonalny UI Test Engine, JUnit XML, poprawny exit code, smoke test podstawowych okien oraz co najmniej jeden test rzeczywistej interakcji widgetowej.

**Powód:** najpierw trzeba potwierdzić stabilną integrację Test Engine z istniejącym GLFW + OpenGL + ImPlot, zanim automatyzacja obejmie gesty na samym wykresie, RMB hit-test i przyszłe kursory/markery.

### D-045 — GitHub Actions jako dodatkowy fallback testowy

**Decyzja:** GitHub Actions jest dodatkową, niezależną ścieżką weryfikacji używaną, gdy lokalny komputer z CANStatio Test Agent jest niedostępny oraz przy ważnych checkpointach projektu.

Workflow na standardowym runnerze Windows buduje pełny projekt w MSYS2 UCRT64 i uruchamia testy funkcyjne przez CTest. Nie zastępuje Test Agenta jako docelowego środowiska GUI.

**Powód:** hostowany runner odtworzył praktycznie ten sam toolchain co komputer docelowy (`GCC 16.2.0`, `CMake/CTest 4.4.3`, `Ninja 1.13.2`, `GTest 1.18.0`) i poprawnie zbudował oba executable oraz przeszedł pełne `17/17` testów funkcyjnych.

### D-046 — GUI Test Engine pozostaje na Test Agencie

**Decyzja:** standardowy GitHub-hosted Windows runner nie jest bazowym środowiskiem wykonywania testów GUI Dear ImGui Test Engine.

**Powód:** eksperymentalne uruchomienie pełnego GUI executable na hostowanym Windowsie zakończyło się przed startem testów błędem GLFW `65542` (`WGL: The driver does not appear to support OpenGL`). Jest to ograniczenie środowiska runnera, a nie regresja aplikacji.

**Konsekwencja:** GitHub Actions sprawdza pełną kompilację GUI, ale rzeczywiste testy interakcji GUI pozostają odpowiedzialnością lokalnego Test Agenta. Nie dokładamy OSMesa/ANGLE ani innych obejść tylko po to, aby wymusić GUI na hostowanym runnerze, dopóki nie pojawi się konkretna potrzeba.

### D-047 — Linux GitHub Actions jako test przenośności i dodatkowa regresja GUI

**Decyzja:** dodajemy osobny workflow `linux-portability.yml`, który na `ubuntu-latest` wykonuje pełny build, testy funkcyjne oraz testy Dear ImGui Test Engine pod `Xvfb` i programowym Mesa (`LIBGL_ALWAYS_SOFTWARE=1`).

**Powód:** zweryfikowany GitHub-hosted Linux runner poprawnie skonfigurował i zbudował cały projekt bez zmian w kodzie C++, a następnie przeszedł oba zestawy testów. Daje to niezależny sygnał przenośności oraz możliwość wykonania GUI regression, gdy docelowy Windows PC jest niedostępny.

**Konsekwencja:** Linux GUI PASS jest dodatkowym testem przenośności, a nie zamiennikiem GUI PASS na docelowym Windows PC. D-046 pozostaje prawdziwe dla standardowego hostowanego runnera Windows oraz dla wymogu target-platform validation; D-047 rozszerza GitHub Actions o niezależną linuxową ścieżkę GUI.

### D-048 — app-owned CursorModel + publiczny ImPlot DragLineX

**Decyzja:** kursory A/B przechowują semantyczny stan czasu i widoczności w małym produkcyjnym `CursorModel`, natomiast render i bezpośredni drag korzystają z publicznego `ImPlot::DragLineX`. `CursorModel` centralizuje clamp czasu do datasetu i jest używany zarówno przez aplikację, jak i testy funkcyjne.

Gdy gest drag zaczyna się na istniejącym kursorze, kursor przejmuje LMB również przy `Alt`, a aktywne Y nie jest wtedy przesuwane. `Shift + LMB` i `Ctrl + LMB` ustawiają odpowiednio A/B, a `Ctrl + RMB` przy linii ukrywa trafiony kursor.

**Powód:** rozwiązanie zachowuje czas jako własność aplikacji, wykorzystuje istniejące publiczne narzędzie ImPlot do geometrii/drag i nie wymaga `implot_internal.h` ani osobnej dużej warstwy tylko dla testowalności.

**Weryfikacja:** po wdrożeniu Stage 6A zestaw wzrósł do `21/21` functional oraz `18/18` GUI na docelowym Windows; Linux portability również przeszedł `21/21 + 18/18`, a GitHub-hosted Windows `21/21` functional.

### D-049 — plain LMB jest docelowym drag kursora A/B

**Decyzja:** istniejący kursor A lub B przeciąga się zwykłym `LMB`, bez wymagania `Alt` ani innego modyfikatora. `Shift + LMB` i `Ctrl + LMB` pozostają gestami ustawiania odpowiednio A i B w miejscu kliknięcia. `Alt` nie jest wymagany do normalnego dragowania kursora.

Jeżeli `Alt + drag` rozpocznie się bezpośrednio na istniejącym kursorze, kursor nadal ma pierwszeństwo przed pan Y, ale jest to reguła rozstrzygania konfliktu wejścia, a nie podstawowy sposób obsługi kursora.

**Powód:** podczas ręcznego testu użytkownik ocenił plain-LMB drag jako wygodniejszy i bardziej naturalny. Bieżąca implementacja oraz `PROJECT_SPEC.md` już odpowiadają temu zachowaniu; decyzja zapisuje je jawnie jako docelowy UX, a nie przypadkowy efekt implementacji.

### D-050 — Test Agent jest jedyną normalną ścieżką, GitHub Windows tylko awaryjnie

**Decyzja:** docelowy Windows PC sterowany przez CANStatio Test Agent jest normalną ścieżką configure/build/functional/GUI. GitHub-hosted Windows pozostaje wyłącznie ręcznym fallbackiem używanym wtedy, gdy target PC lub Test Agent jest niedostępny. Workflow Windows ma tylko `workflow_dispatch`. Linux GitHub Actions był eksperymentem i jego workflow został całkowicie usunięty z projektu.

**Zastępuje operacyjnie:** D-045 w części dotyczącej checkpointów oraz D-047. Historyczne wyniki tych eksperymentów pozostają zapisane, ale nie definiują bieżącej strategii.

### D-051 — stały app-owned gutter semantycznej osi Y

**Decyzja:** natywne dekoracje Y ImPlot są zawsze wyłączone. Aplikacja stale rezerwuje po lewej gutter semantycznej osi Y o szerokości wynikającej z fontu i stałego referencyjnego budżetu etykiety. Tick marks i etykiety aktywnej serii są rysowane przez publiczny ImGui DrawList. Gdy nie ma active visible series, gutter pozostaje pusty, ale nadal zarezerwowany.

**Powód:** automatyczna szerokość dekoracji Y ImPlot powodowała nieprzyjemną zmianę położenia i szerokości plot area przy aktywacji/dezaktywacji serii.

**Weryfikacja:** GUI regression `stable_plot_area_active_y_axis` potwierdza niezmienne `plotPos`/`plotSize` dla braku active, różnych active series i ponownego wyłączenia active.

### D-052 — Halo / Outline jako jedna screen-space polyline

**Decyzja:** właściwa linia active pozostaje renderowana przez ImPlot z grubością 1.5 px. Halo i Outline nie są już drugim grubym `PlotLineG`; są rysowane jako jedna połączona screen-space polyline przez publiczny `ImDrawList::AddPolyline`, z clippingiem do plot area i tylko dla zakresu X potrzebnego w bieżącym widoku plus zapas przy krawędziach.

**Powód:** gruby `PlotLineG` tworzył nieestetyczne szczeliny lub spłaszczenia na łączeniach segmentów. Jedna polyline ma poprawną wspólną geometrię joinów i zachowuje overlay jako czysto wizualną warstwę bez wpływu na Fit, legendę ani hit-test.

**Zastępuje techniczny sposób renderowania warstwy wyróżnienia opisany w D-038;** parametry wizualne Halo/Outline z D-038 pozostają aktualne.

### D-053 — Alt+drag należy do Y-pan, także nad kursorem

**Decyzja:** cursor drag działa wyłącznie przy plain LMB bez modyfikatora. `Alt + drag` jest zarezerwowany dla active-Y pan i nie przeciąga kursora nawet wtedy, gdy gest zaczyna się nad jego linią. Ta sama zasada ma obowiązywać przyszłe markery: ich drag również będzie plain LMB.

**Powód:** plain-LMB drag został ręcznie oceniony jako naturalniejszy, a jednoznaczna własność `Alt` upraszcza mapę wejścia i przyszłe rozróżnienie `Alt-click` marker vs `Alt-drag` Y.

**Zastępuje:** regułę pierwszeństwa kursora przy `Alt + drag` z D-048 oraz drugiego akapitu D-049.

**Weryfikacja:** GUI test `cursors/alt_drag_over_cursor_pans_y` potwierdza, że kursor pozostaje w tym samym czasie, X pozostaje nieruchomy, a active Y jest przesuwane.
