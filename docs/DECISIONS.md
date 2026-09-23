# Rejestr decyzji

Ten plik zapisuje aktualne decyzje eksperymentu `CANStatio-ImGui-custom-test`.

Skopiowana baza kodu jest tylko punktem startowym. Decyzje wynikające z poprzedniej architektury nie obowiązują tutaj, jeśli nie zostały jawnie zachowane poniżej.

## 2026-09-23

### D-001 — osobne repozytorium dla własnego wykresu

**Decyzja:** eksperyment własnej implementacji wykresu jest prowadzony w osobnym repozytorium `CANStatio-ImGui-custom-test`.

### D-002 — Dear ImGui pozostaje frameworkiem GUI

**Decyzja:** Dear ImGui pozostaje podstawową warstwą GUI aplikacji.

### D-003 — własna implementacja wykresu

**Decyzja:** docelowa warstwa wykresu nie korzysta z dodatkowej biblioteki wykresowej. Aplikacja przejmuje layout, transformacje, render, hit-test i input.

### D-004 — skopiowany kod nie jest źródłem wymagań technicznych

**Decyzja:** istniejąca implementacja jest bazą migracyjną i materiałem referencyjnym dla zachowania użytkowego. Jej rozwiązania techniczne nie są wymaganiami nowego renderera.

### D-005 — regularny wspólny X

**Decyzja:** wszystkie serie w pierwszym eksperymencie mają wspólne regularne timestampy `x = index * dt`.

### D-006 — dane Y jako `float`

**Decyzja:** pierwsza wersja używa ciągłych serii `float`. Step/digital i inne typy danych pozostają poza pierwszym zakresem.

### D-007 — deterministic generator

**Decyzja:** każdy preset ma stały seed i daje te same dane przy każdym uruchomieniu.

### D-008 — raw data first

**Decyzja:** pierwsza implementacja nie używa LOD/downsamplingu. Optymalizacja dopiero po pomiarze.

### D-009 — niezależny stan Y każdej serii

**Decyzja:** każda seria przechowuje własne `viewMin/viewMax`. Zmiana active nie resetuje Y.

### D-010 — bez wspólnej sztucznej przestrzeni Y

**Decyzja:** renderer mapuje surową wartość każdej serii bezpośrednio z jej `viewMin..viewMax` do pikseli plot area.

### D-011 — app-owned plot area i transformacje

**Decyzja:** aplikacja jest właścicielem plot rectangle oraz funkcji data-to-screen i screen-to-data. Ten sam system współrzędnych jest używany przez rendering, hit-test, Values, crosshair, cursory i markery.

### D-012 — app-owned X navigation

**Decyzja:** plain LMB drag realizuje pan X, a plain wheel zoom X, o ile gest nie został przejęty przez narzędzie. `Fit X` ustawia pełny dataset.

### D-013 — app-owned Y navigation

**Decyzja:** `Alt + drag` przesuwa Y active visible series, a `Alt + wheel` skaluje jej zakres Y. X pozostaje nieruchomy.

### D-014 — własne osie, ticki i grid

**Decyzja:** aplikacja generuje i renderuje osie, etykiety, ticki i grid. Plot area ma zachowywać stałą geometrię przy zmianie active series.

### D-015 — jedna semantyczna oś Y aktywnej serii

**Decyzja:** tylko active visible series prezentuje semantic Y i dodatkowy kolorowy grid.

### D-016 — aktywna seria przez RMB hit-test

**Decyzja:** plain RMB w pobliżu widocznej geometrii serii przełącza active. RMB poza seriami czyści active. Hit-test pracuje w screen-space i używa około 5 px tolerancji.

### D-017 — Custom Legend jako jedyna legenda funkcjonalna

**Decyzja:** Custom Legend pokazuje wszystkie serie, steruje visibility i pozwala przełączać active. Nie ma drugiej legendy renderera.

### D-018 — cienka linia active + Halo / Outline

**Decyzja:** właściwa linia active ma około 1.5 px. Opcjonalny highlight pod nią:

- `Halo` około 7 px, kolor serii, alpha około 0.22;
- `Outline` około 5 px, neutralny kolor tekstu, alpha około 0.65.

Highlight nie zmienia hit-testu.

### D-019 — Values używa interpolacji liniowej

**Decyzja:** wartości pod dokładnym mouse X są interpolowane liniowo pomiędzy sąsiednimi regularnymi próbkami. Crosshair nie snapuje do próbki.

### D-020 — dokładnie dwa kursory A/B

**Decyzja:** A i B przechowują czas i widoczność jako app-owned state.

- `Shift + LMB` ustawia A;
- `Ctrl + LMB` ustawia B;
- plain LMB przeciąga istniejący kursor;
- `Ctrl + RMB` ukrywa trafiony kursor;
- czas jest clampowany do datasetu.

### D-021 — markery numerowane od 0

**Decyzja:** marker labels to `0, 1, 2, ...`; usunięty numer nie jest używany ponownie; Reset zeruje licznik.

### D-022 — Alt-click vs Alt-drag przez threshold

**Decyzja:** `Alt + LMB` poniżej progu ruchu około 4–5 px tworzy marker. Po przekroczeniu progu gest staje się pan Y, jeśli istnieje active visible series; w przeciwnym razie gest jest anulowany.

### D-023 — jednoznaczny routing modyfikatorów

**Decyzja:** modifier gestures mają pierwszeństwo przed normalną nawigacją X zgodnie z mapą z `PROJECT_SPEC.md`.

### D-024 — automatyzacja nie zastępuje ręcznej oceny UX

**Decyzja:** testy automatyczne obejmują logikę, stan, transformacje i deterministyczne interakcje. Wygląd, ergonomia i płynność wymagają ręcznej weryfikacji.

### D-025 — testy functional i GUI pozostają

**Decyzja:** GoogleTest + CTest pozostają warstwą functional, a Dear ImGui Test Engine warstwą GUI integration.

### D-026 — Test Agent jako podstawowa ścieżka

**Decyzja:** docelowy Windows PC przez CANStatio Test Agent jest normalną ścieżką configure/build/functional/GUI. GitHub-hosted Windows pozostaje ręcznym fallbackiem.

### D-027 — granica repozytorium

**Decyzja:** w ramach tej rozmowy modyfikacje są dozwolone wyłącznie w `CANStatio-ImGui-custom-test`. Inne repozytoria nie mogą być zmieniane. Test Agent może być używany jedynie jako kanał komunikacji ze środowiskiem testowym.

### D-028 — zoom X względem myszy

**Decyzja:** plain wheel nad plot area skaluje X względem czasu pod kursorem. Punkt czasu pod myszą pozostaje pod tym samym pikselem po zoomie.

Bazowy krok zoomu jest multiplikatywny, około `1.15` na jednostkę wheel. Dodatni wheel oznacza zoom in.

**Powód:** zachowanie jest intuicyjne przy analizie konkretnego fragmentu przebiegu i nie wymaga dodatkowego trybu ani ustawień.

### D-029 — minimalny X bez sztucznego maksymalnego zoom-out

**Decyzja:** minimalny sensowny span X to jeden krok próbkowania `dt`. Viewport X nie jest clampowany do datasetu i nie ma sztucznego maksymalnego zoom-out.

Niepoprawny albo niefinity stan X jest odzyskiwany przez Fit X.

**Powód:** `dt` jest naturalną granicą rozdzielczości czasu dla pierwszego modelu danych. Dalsze limity nie rozwiązują problemu eksperymentu.

### D-030 — zoom Y wokół środka zakresu

**Decyzja:** `Alt + wheel` zoomuje Y active visible series wokół środka jej `viewMin/viewMax`, używając tego samego prostego multiplikatywnego kroku około `1.15`.

Zakres Y ma tylko techniczne zabezpieczenie przed zerowym/niefinitym spanem; nie ma sztucznych użytkowych limitów.

**Powód:** zachowujemy prostą i stabilną semantykę niezależną od położenia myszy.

### D-031 — nice ticks Y = 1/2/5 × 10^n

**Decyzja:** semantic Y ticks korzystają z klasycznego szeregu `1, 2, 5 × 10^n` i są dobierane do wysokości plot area, z docelowym odstępem około `60–80 px`.

Neutralne poziome linie grida są tylko wizualnym podziałem i nie reprezentują żadnej serii.

**Powód:** to prosty, przewidywalny algorytm bez rozbudowanej heurystyki.

### D-032 — gesty z modyfikatorem są wyłączne

**Decyzja:** gest z `Shift`, `Ctrl` lub `Alt` nie przechodzi do zwykłego pan X ani RMB selection, jeżeli jego własna akcja nie może zostać wykonana.

W szczególności:

- `Alt + drag/wheel` bez active visible series nie rusza X;
- `Ctrl + RMB` bez trafienia w cursor/marker jest ignorowany;
- przekroczenie progu Alt-drag bez active visible series anuluje marker i nie wykonuje pan X.

**Powód:** eliminuje nieoczekiwane akcje wynikające z fallbacków inputu.

### D-033 — input rozpoczyna się nad plot area i zachowuje ownership do release

**Decyzja:** gest wykresu może rozpocząć się tylko nad plot area. Po rozpoczęciu drag wybrany handler zachowuje ownership aż do release, również po wyjściu kursora poza plot area.

**Powód:** prosta i standardowa semantyka drag bez przerywania operacji na krawędzi wykresu.

### D-034 — grab-content dla pan X/Y

**Decyzja:** pan X i pan Y stosują zachowanie „grab content”: wizualna zawartość podąża za ruchem myszy.

**Powód:** daje spójny mental model dla obu osi i upraszcza testowanie kierunku przesunięcia.

### D-035 — specyfikacja bazowa zamknięta

**Decyzja:** bieżąca wersja `docs/PROJECT_SPEC.md` jest kompletną specyfikacją bazową pierwszego eksperymentu custom chart.

Drobne parametry wizualne i tuning szybkości mogą zmieniać się po testach manualnych bez zmiany specyfikacji, ale zmiana semantyki gestów, modelu danych, zakresu eksperymentu lub zachowania narzędzi wymaga jawnej aktualizacji `PROJECT_SPEC.md` i tego rejestru.