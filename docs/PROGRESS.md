# Postęp prac

## Stan na 2026-09-23

Normalny branch rozwojowy:

```text
main
```

Eksperyment infrastrukturalny `feature/imgui-test-engine` został zakończony i zintegrowany do `main` przez PR #1. Merge wykonano jako squash, aby zachować liniowy charakter głównej historii i potraktować cały eksperyment jako jeden logiczny checkpoint.

Projekt ma trzy poziomy weryfikacji:

1. functional C++ — GoogleTest + CTest;
2. GUI integration — Dear ImGui Test Engine;
3. manual — wygląd, czytelność, ergonomia i subiektywna płynność.

Podstawową ścieżką automatycznej weryfikacji jest **docelowy Windows PC przez CANStatio Test Agent**. GitHub-hosted Windows pozostaje wyłącznie ręcznym fallbackiem na sytuację, gdy target PC/Test Agent jest niedostępny. Linux GitHub Actions był eksperymentem i został usunięty z projektu.

## Aktualny wynik automatyczny

Ostatnia pełna weryfikacja kodu przed integracją do `main`:

```text
Test Agent Windows: 21/21 functional PASS + 19/19 GUI PASS
```

Po tej weryfikacji przed merge zmieniała się wyłącznie dokumentacja. Po merge bieżące zmiany porządkowe na `main` również dotyczą wyłącznie dokumentacji.

Awaryjny GitHub-hosted Windows został wcześniej zweryfikowany jako pełny build + `21/21 functional PASS`, ale nie jest normalną ścieżką testową.

## Functional — 21 testów

Pokrycie:

- `Dataset::endTimeSeconds()`;
- `Series::fitViewToData()`;
- `ValuesModel` — exact sample, interpolacja, granice, hidden/no-hover/outside dataset i edge-case'y;
- `CursorModel` — start hidden, niezależne A/B, clamp do datasetu i niezależne hide.

## GUI integration — 19 testów

Zestaw obejmuje startup, okna, legendę, active/hidden-active, Halo/Outline mode, Fit X/Y, X navigation, `Alt` Y navigation, Values/crosshair, RMB hit-test, kursory A/B oraz stabilność geometrii plot area przy zmianach active Y.

Testy kursora:

```text
cursors/set_a_and_b
cursors/plain_drag_and_ctrl_rmb_remove
cursors/alt_drag_over_cursor_pans_y
```

`Alt + drag` nie ma specjalnej obsługi kursora — test tylko chroni ogólną zasadę, że przy `Alt` kursor nie reaguje, a aktywne Y jest przesuwane.

Test layoutu:

```text
canstatio/stable_plot_area_active_y_axis
```

sprawdza stałe `plotPos`/`plotSize` przy braku active, aktywacji, zmianie active na serię o innej skali i ponownym wyłączeniu active.

## Stage 6A — kursory A/B

**Zaimplementowany, automatycznie zweryfikowany i ręcznie zaakceptowany.**

- dokładnie dwa kursory `A` i `B`;
- app-owned time state w `CursorModel`;
- render/drag przez publiczne `ImPlot::DragLineX`;
- `Shift + LMB` — ustaw/przenieś A;
- `Ctrl + LMB` — ustaw/przenieś B;
- plain LMB na kursorze — drag;
- `Ctrl + RMB` przy kursorze — hide;
- clamp czasu do `0..dataset.endTimeSeconds()`;
- dwa osobne kolory i etykiety A/B;
- `Alt + drag` jest zwykłym pan Y i nie przeciąga kursora.

Nie zaimplementowano osobnego panelu/wyświetlania `B-A`.

## Stabilizacja layoutu Y

Naprawiono stary problem zmiany szerokości plot area podczas aktywacji/dezaktywacji serii.

Aktualne rozwiązanie:

- natywne dekoracje Y ImPlot są wyłączone;
- aplikacja stale rezerwuje lewy semantyczny gutter Y;
- jego szerokość wynika z fontu oraz stałego referencyjnego budżetu etykiety, a nie z aktualnych wartości serii;
- bez active series gutter jest pusty, ale nadal zajmuje tę samą szerokość;
- tick marks i liczby aktywnej serii są rysowane przez publiczny ImGui DrawList;
- fizyczna oś ImPlot pozostaje `0..1`.

Automatyczna regresja potwierdza stałą geometrię plot area. Użytkownik ręcznie zaakceptował końcowy wygląd i zachowanie.

## Halo / Outline — poprawa łączeń

Stary wariant używał drugiego grubego `ImPlot::PlotLineG`, co przy załamaniach tworzyło szczeliny lub spłaszczenia.

Aktualne rozwiązanie:

- właściwa linia active nadal jest rysowana przez ImPlot i ma 1.5 px;
- Halo 7 px / alpha ~0.22 i Outline 5 px / alpha ~0.65 są rysowane jako jedna screen-space polyline przez publiczny `ImDrawList::AddPolyline`;
- overlay używa tylko zakresu X potrzebnego do bieżącego widoku plus zapas przy krawędziach;
- overlay jest clippingowany do plot area;
- nie wpływa na legendę, Fit ani hit-test.

Zmiana przeszła build + pełny GUI suite i została ręcznie zaakceptowana wizualnie.

## GitHub Actions — awaryjny Windows fallback

Jedyny bieżący workflow:

```text
.github/workflows/windows-ci.yml
```

Zasada:

```text
Test Agent dostępny     -> użyj Test Agenta, nie GitHub Actions
Test Agent niedostępny  -> można ręcznie uruchomić GitHub Windows fallback
```

Workflow ma tylko `workflow_dispatch`; brak automatycznych triggerów push/PR.

GUI na hostowanym Windows nie jest wykonywane z powodu braku użytecznego WGL/OpenGL w standardowym runnerze.

## Stan etapów aplikacji

- **Etap 1 — dataset/render:** zaimplementowany;
- **Etap 2 — niezależny Y:** zaimplementowany, z ustabilizowanym semantic-Y gutter;
- **Etap 3 — X/time axis:** zaimplementowany;
- **Etap 4 — selection / legend:** zaimplementowany;
- **Etap 5 — Values / crosshair:** zaimplementowany;
- **Etap 6A — A/B cursors:** zakończony;
- **Etap 6B — markery / input priority:** jeszcze niezaimplementowany.

## Nadal niezaimplementowane

- pełne presety Reference / Overlap / Mixed Scale / Spikes / Long Time / Stress Raw;
- minor grid X;
- markery;
- `Alt-click` marker vs `Alt-drag` Y threshold i pozostałe input-priority conflicts;
- pełny Reset / preset switching;
- testy wydajności docelowych presetów.

## Najbliższy krok

Rozpocząć planowanie **Stage 6B — markery** na `main`.

Docelowa koncepcja pozostaje:

- `Alt + LMB` click — utworzenie markera;
- `Alt + drag` — pan Y;
- próg ruchu około 4–5 px rozstrzyga click vs drag;
- plain LMB na istniejącym markerze — drag;
- `Ctrl + RMB` — remove;
- numeracja 0,1,2... bez ponownego używania numerów do pełnego Reset.

Przed implementacją należy jeszcze doprecyzować routing inputu markerów względem kursora i natywnego X, ale bez tworzenia specjalnej semantyki dla `Alt + drag` nad kursorem.
