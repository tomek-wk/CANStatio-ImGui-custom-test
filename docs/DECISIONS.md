# Rejestr decyzji

Ten plik zapisuje aktualne decyzje eksperymentu `CANStatio-ImGui-custom-test`.

Skopiowana baza kodu jest tylko punktem startowym. Decyzje wynikające z poprzedniej architektury nie obowiązują tutaj, jeśli nie zostały jawnie zachowane poniżej.

## 2026-09-23

### D-001 — osobne repozytorium dla własnego wykresu

**Decyzja:** eksperyment własnej implementacji wykresu jest prowadzony w osobnym repozytorium `CANStatio-ImGui-custom-test`.

**Powód:** wariant ma swobodnie zmieniać renderer, input, layout i strukturę bez wpływu na inne eksperymenty ani główny CANStatio.

### D-002 — Dear ImGui pozostaje frameworkiem GUI

**Decyzja:** Dear ImGui pozostaje podstawową warstwą GUI aplikacji.

**Powód:** wcześniejsze próby potwierdziły, że odpowiada potrzebom planowanego LogViewera jako framework interfejsu.

### D-003 — własna implementacja wykresu

**Decyzja:** docelowa warstwa wykresu nie korzysta z dodatkowej biblioteki wykresowej.

Aplikacja przejmuje odpowiedzialność za layout, transformacje, render, hit-test i input wykresu.

### D-004 — skopiowany kod nie jest źródłem wymagań technicznych

**Decyzja:** istniejąca implementacja w repozytorium jest bazą migracyjną i materiałem referencyjnym dla zachowania użytkowego.

Jej rozwiązania techniczne nie są wymaganiami dla nowego renderera.

**Powód:** celem eksperymentu jest sprawdzenie innej architektury, a nie odtworzenie poprzedniego backendu innymi funkcjami.

### D-005 — regularny wspólny X

**Decyzja:** wszystkie serie w pierwszym eksperymencie mają wspólne regularne timestampy `x = index * dt`.

**Powód:** pozwala skupić eksperyment na rendererze, input routing i niezależnym Y.

### D-006 — dane Y jako `float`

**Decyzja:** pierwsza wersja używa ciągłych serii `float`.

Step/digital i inne typy danych pozostają poza pierwszym zakresem.

### D-007 — deterministic generator

**Decyzja:** każdy preset ma stały seed i daje te same dane przy każdym uruchomieniu.

**Powód:** ułatwia testy regresji, porównania wizualne i pomiary wydajności.

### D-008 — raw data first

**Decyzja:** pierwsza implementacja nie używa LOD/downsamplingu.

**Powód:** najpierw potrzebny jest prosty punkt odniesienia i rzeczywisty pomiar własnego renderera.

### D-009 — niezależny stan Y każdej serii

**Decyzja:** każda seria przechowuje własne `viewMin/viewMax`.

Zmiana active nie resetuje zakresu Y serii.

### D-010 — bez wspólnej sztucznej przestrzeni Y

**Decyzja:** renderer mapuje surową wartość każdej serii bezpośrednio z jej `viewMin..viewMax` do pikseli plot area.

Nie utrzymujemy wspólnej logicznej osi Y dla wszystkich serii tylko po to, aby ujednolicić renderer.

### D-011 — app-owned plot area i transformacje

**Decyzja:** aplikacja jest właścicielem plot rectangle oraz funkcji data-to-screen i screen-to-data.

**Konsekwencja:** ten sam system współrzędnych ma być używany przez rendering, hit-test, Values, crosshair, cursory i markery.

### D-012 — app-owned X navigation

**Decyzja:** plain LMB drag realizuje pan X, a plain wheel zoom X, o ile gest nie został przejęty przez narzędzie.

`Fit X` ustawia pełny czas datasetu.

### D-013 — app-owned Y navigation

**Decyzja:** `Alt + drag` przesuwa Y aktywnej widocznej serii, a `Alt + wheel` skaluje jej zakres Y.

X pozostaje nieruchomy podczas operacji Y.

### D-014 — własne osie, ticki i grid

**Decyzja:** aplikacja generuje i renderuje osie, etykiety, ticki i grid.

Plot area ma zachowywać stałą geometrię przy zmianie active series. Layout osi ma być przewidywalny i całkowicie kontrolowany przez aplikację.

### D-015 — jedna semantyczna oś Y aktywnej serii

**Decyzja:** tylko aktywna widoczna seria prezentuje opis wartości Y i dodatkowy kolorowy grid.

Nie pokazujemy jednocześnie wielu semantycznych osi Y.

### D-016 — aktywna seria przez RMB hit-test

**Decyzja:** plain RMB w pobliżu widocznej geometrii serii przełącza active. RMB poza seriami czyści active.

Hit-test pracuje w screen-space i wybiera najbliższy segment w tolerancji około 5 px.

### D-017 — Custom Legend jako jedyna legenda funkcjonalna

**Decyzja:** Custom Legend pokazuje wszystkie serie, steruje visibility i pozwala przełączać active.

Nie planujemy drugiej legendy będącej częścią renderera wykresu.

### D-018 — cienka linia active + Halo / Outline

**Decyzja:** właściwa linia aktywnej serii pozostaje cienka, około 1.5 px. Opcjonalne wyróżnienie jest rysowane pod nią:

- `Halo` około 7 px, kolor serii, alpha około 0.22;
- `Outline` około 5 px, neutralny kolor tekstu, alpha około 0.65.

Highlight i linia danych korzystają z tej samej geometrii screen-space i nie zmieniają hit-testu.

### D-019 — Values używa interpolacji liniowej

**Decyzja:** wartości pod dokładnym mouse X są interpolowane liniowo pomiędzy sąsiednimi regularnymi próbkami.

Crosshair jest niezależną pionową linią czasu i nie snapuje do próbek.

### D-020 — dokładnie dwa kursory A/B

**Decyzja:** A i B przechowują czas i widoczność jako app-owned state.

- `Shift + LMB` ustawia A;
- `Ctrl + LMB` ustawia B;
- plain LMB przeciąga istniejący kursor;
- `Ctrl + RMB` ukrywa trafiony kursor;
- czas jest clampowany do datasetu.

Render i drag korzystają z własnego systemu współrzędnych wykresu.

### D-021 — markery numerowane od 0

**Decyzja:** marker labels to `0, 1, 2, ...`; usunięty numer nie jest używany ponownie; Reset zeruje licznik.

### D-022 — Alt-click vs Alt-drag przez threshold

**Decyzja:** `Alt + LMB` poniżej progu ruchu około 4–5 px tworzy marker. Po przekroczeniu progu gest staje się pan Y i marker nie powstaje.

### D-023 — jednoznaczny routing modyfikatorów

**Decyzja:** modifier gestures mają pierwszeństwo przed normalną nawigacją X zgodnie z mapą z `PROJECT_SPEC.md`.

Routing jest własnością aplikacji i nie zależy od zachowania zewnętrznego plot widgetu.

### D-024 — automatyzacja nie zastępuje ręcznej oceny UX

**Decyzja:** testy automatyczne obejmują logikę, stan, transformacje i deterministyczne interakcje. Wygląd, czytelność, ergonomia i płynność nadal wymagają ręcznej weryfikacji.

### D-025 — testy functional i GUI pozostają

**Decyzja:** GoogleTest + CTest pozostają warstwą functional, a Dear ImGui Test Engine warstwą GUI integration.

Nie tworzymy nowego frameworka testów tylko z powodu zmiany renderera.

### D-026 — Test Agent jako podstawowa ścieżka

**Decyzja:** docelowy Windows PC sterowany przez CANStatio Test Agent jest normalną ścieżką configure/build/functional/GUI.

GitHub-hosted Windows pozostaje tylko ręcznym fallbackiem, gdy target PC/Test Agent jest niedostępny.

### D-027 — granica repozytorium

**Decyzja:** w ramach tej rozmowy i prac nad eksperymentem modyfikacje są dozwolone wyłącznie w `CANStatio-ImGui-custom-test`.

Inne repozytoria nie mogą być zmieniane. Test Agent może być używany jedynie jako kanał komunikacji ze środowiskiem testowym.