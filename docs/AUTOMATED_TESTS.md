# Automatyczne testy

## Cel

Automatyzacja ma wykrywać regresje funkcjonalne eksperymentu Dear ImGui z własną implementacją wykresu bez zastępowania ręcznej oceny wyglądu, ergonomii i UX.

Strategia ma trzy warstwy:

1. **functional C++** — logika, matematyka, transformacje, stan i edge-case'y bez GUI;
2. **GUI integration** — rzeczywiste interakcje Dear ImGui przez Dear ImGui Test Engine;
3. **manual** — wygląd, czytelność, ergonomia, subiektywna płynność i UX.

Ten dokument jest nadrzędnym źródłem prawdy w zakresie automatycznych testów, ich narzędzi i uruchamiania.

## Zasady wyboru narzędzi

- nie tworzymy osobnej biblioteki `core` tylko dla testów;
- nie przenosimy FlaUI / UIA3 / xUnit ze starego wxWidgets;
- nie dodajemy nowej zależności, jeśli istniejące narzędzia rozwiązują problem;
- najpierw testujemy istniejący produkcyjny interfejs;
- małą czystą funkcję lub model wydzielamy tylko wtedy, gdy zapobiega to duplikowaniu algorytmu albo kruchemu testowi GUI i ma sens produkcyjny;
- matematyka data/screen, hit-test i pan/zoom powinna być testowana funkcyjnie tam, gdzie można ją oddzielić od samego renderowania.

## Narzędzia

### Functional

Docelowy Windows PC:

```text
CMake 4.4.3
CTest 4.4.3
GoogleTest 1.18.0
GCC/G++ 16.2.0
Ninja 1.13.2
```

GoogleTest jest znajdowany przez `find_package(GTest REQUIRED)`; repo go nie pobiera.

### GUI

Framework: **Dear ImGui Test Engine**.

Bazowe wersje:

- Dear ImGui `v1.92.9b`;
- Dear ImGui Test Engine commit `508a8fc8dacac2f346d353fed31b9bc90ed29adc`.

Tryby i nazwy executable mogą ulec zmianie podczas etapu C1. Dokument nie utrwala nazw skopiowanego targetu jako części nowej architektury.

Nie dodajemy OS-level GUI automation bez konkretnej luki, której Dear ImGui Test Engine nie potrafi wiarygodnie sprawdzić.

## CANStatio Test Agent — podstawowa ścieżka

CANStatio Test Agent jest podstawowym zdalnym środowiskiem testowym docelowego Windows PC.

Normalny przepływ:

```text
configure/build
-> functional
-> GUI
-> manual UX, jeśli zmiana dotyczy interakcji lub wyglądu
```

Repozytorium Test Agenta nie jest modyfikowane przez ten projekt. Może być używane tylko jako kanał komunikacji z docelowym środowiskiem.

## GitHub Actions — tylko awaryjny Windows fallback

GitHub Actions nie jest normalną ścieżką testową i nie jest dodatkowym obowiązkowym checkpointem.

Używamy go wyłącznie wtedy, gdy docelowy Windows PC lub Test Agent jest niedostępny.

Workflow:

```text
.github/workflows/windows-ci.yml
```

Jest celowo `workflow_dispatch` only — bez triggerów `push` i `pull_request`.

Standardowy GitHub-hosted Windows runner nie jest podstawowym środowiskiem GUI. Nie dokładamy alternatywnych backendów renderingu tylko po to, aby wymusić testy GUI na runnerze.

Linux GitHub Actions nie jest częścią bieżącej strategii testowej.

## Historyczny baseline przed migracją

Skopiowana baza projektu miała potwierdzony wynik:

```text
Test Agent Windows:    21/21 functional PASS
Test Agent Windows:    19/19 GUI PASS
GitHub Windows:        21/21 functional PASS — historycznie zweryfikowany fallback
```

Te wyniki nie są wynikiem własnego renderera. Służą wyłącznie jako punkt odniesienia dla zachowań, które warto zachować.

Po migracji test dotkniętego obszaru liczy się jako zweryfikowany dopiero po ponownym uruchomieniu przeciwko custom chart.

## Co można zachować z istniejących testów

Najbardziej prawdopodobne do bezpośredniego zachowania lub małej korekty:

- `Dataset::endTimeSeconds()`;
- `Series::fitViewToData()`;
- `ValuesModel` — exact sample, interpolacja, hidden/no-hover/outside dataset i edge-case'y;
- `CursorModel` — start hidden, niezależne A/B, clamp i hide.

GUI testy zależne od starego plot widgetu mogą wymagać przepisania, nawet jeśli ich semantyka użytkowa pozostaje taka sama.

## Plan testów functional

### F0 — środowisko + runner

Status: istniejąca infrastruktura dostępna, do ponownego potwierdzenia po zmianie targetów w C1.

### F1 — Dataset / Series

Status: baseline istnieje.

Zakres:

- `endTimeSeconds()`;
- Fit Y margin;
- constant-value edge cases.

### F2 — transformacja X

Do dodania razem z własnym rendererem.

Zakres:

- time -> screen X;
- screen X -> time;
- granice plot rect;
- zoomed/panned ranges;
- wartości poza viewportem.

### F3 — transformacja Y

Do dodania.

Zakres:

- raw value -> screen Y dla niezależnego `viewMin/viewMax`;
- screen Y -> raw value;
- różne skale serii;
- constant/range edge cases;
- wartości poza viewportem.

### F4 — X pan / zoom / Fit X math

Do dodania.

Zakres:

- zachowanie span przy pan;
- zmiana span przy wheel zoom;
- anchoring zgodny z przyjętą semantyką;
- Fit X.

### F5 — Y pan / zoom / Fit Y math

Do dodania lub rozszerzenia.

Zakres:

- pan aktywnej serii bez zmiany X;
- zoom aktywnej serii bez zmiany X;
- minimalny bezpieczny span;
- niezależność Y pomiędzy seriami.

### F6 — ticki i formatowanie czasu/Y

Do dodania.

Zakres:

- wybór czytelnego kroku;
- sekundy/minuty/godziny;
- format milisekund;
- stabilne wyniki przy nietypowych zakresach.

### F7 — RMB hit-test math

Do dodania lub przeniesienia.

Zakres:

- point-to-segment w screen-space;
- najbliższa seria wygrywa;
- hidden series pomijana;
- empty-space clears active;
- duży zoom-out i wiele próbek w jednym pikselu.

### F8 — Values

Status: model baseline istnieje, do ponownego potwierdzenia po podpięciu pod własny mouse-time transform.

### F9 — kursory A/B

Status: state model baseline istnieje.

Do dodania/przepisania:

- screen hit-test kursora;
- drag -> time;
- clamp;
- kolizje wejścia.

### F10 — markery i input priority

Do dodania razem z implementacją.

Zakres:

- numeracja;
- add/remove/drag;
- Alt-click threshold;
- Alt-drag Y pan;
- konflikty cursor/marker.

### F11 — performance helpers

Dopiero po implementacji pełnych presetów, jeśli wydzielimy testowalne elementy związane z widocznym zakresem indeksów lub przygotowaniem geometrii.

## Plan GUI integration

Minimalny zestaw ma docelowo pokrywać:

- startup i okna pomocnicze;
- stabilny plot area;
- Custom Legend visibility i active;
- Fit X / Fit Y;
- plain drag / wheel X;
- Alt drag / wheel Y;
- RMB selection;
- Values i crosshair;
- Halo / Outline mode;
- cursory A/B;
- markery;
- input priority i konflikty modifierów.

Testy GUI powinny sprawdzać semantykę i state, a nie utrwalać niepotrzebnie wewnętrzną strukturę renderera.

## Weryfikacja etapami migracji

### C1

Wymagane:

- configure/build;
- minimalny GUI smoke;
- potwierdzenie, że aplikacja działa bez starego backendu wykresu.

### C2

Wymagane:

- functional transforms X;
- tick/time formatting;
- GUI X pan/zoom/Fit X;
- manualna ocena osi i gridu.

### C3

Wymagane:

- functional Y math i hit-test;
- GUI active/visibility/Fit Y/Y navigation/RMB;
- manual Halo/Outline i semantic Y.

### C4

Wymagane:

- functional Values;
- GUI Values/crosshair.

### C5

Wymagane:

- functional cursor state + geometry helpers;
- GUI set/drag/remove + modifier conflicts;
- manual chwytanie kursora i czytelność etykiet.

### C6

Wymagane:

- functional marker state/input threshold;
- pełny GUI input-priority suite;
- manual ergonomia markerów.

### C7

Wymagane:

- full regression;
- manual performance run na Reference i Stress Raw;
- zapis wyników i ewentualna decyzja o LOD.

## Czego automaty nie zastępują

- jakości wizualnej linii i highlightów;
- czytelności osi i etykiet;
- praktycznej szerokości obszaru osi Y;
- czytelności Values i crosshair;
- kolorów, etykiet i łatwości chwytania cursorów/markerów;
- subiektywnej ergonomii i płynności;
- manualnego sprawdzenia zachowania na rzeczywistym docelowym sprzęcie.