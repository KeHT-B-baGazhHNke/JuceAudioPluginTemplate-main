# Guitar Amp Emulation (JUCE + CMake)

Эмуляция аналогового гитарного усилителя в виде VST3 плагина и Standalone приложения на основе фреймворка [JUCE](https://juce.com)  и встроенной библиотеки DSP.

---

## Функционал

Этот плагин представляет собой полноценный гитарный усилитель в цифровом виде. Он позволяет получить богатый, живой звук электрогитары — такой же, как от настоящего лампового усилителя, подключённого к реальному кабинету.

### Что он делает?

- **Эмуляция перегруза лампового усилителя**: дает насыщенный перегруз, который можно регулировать — от слегка подкрашенного чистого звука до плотного перегруза.
- **Регулировка тембра (Tone)**: простая, но эффективная ручка, которая позволяет сделать звук ярче (при увеличении) или более тёплым и тусклым (при уменьшении).
- **IR-кабинет (Impulse Response)**: имитация акустики реального гитарного кабинета через загрузку .wav файлов.
- **Bypass**: возможность отключить эмуляцию усилителя или кабинета, например для использования стороннего плагина усилителя (или кабинета).
- **Гибкость в использовании**: подходит для игры в разных стилях - как для олдскульного рока, так и для более современных жанров, где важен характерный "грязный", но контролируемый звук.

---

### Почему это удобно?

- Простой интерфейс — всё необходимое под рукой.
- Никаких лишних параметров — только то, что влияет на звук в реальном усилителе.
- Подходит как для записи в DAW, так и для живых выступлений через VST-хост.

---

## Архитектура обработки звука

Сигнал проходит следующие этапы обработки:
[Input] → [High-Pass and Low-Pass Filters] → [Tube Clipping + Interstage Filters] → [Tone Control] → [Mid-Cut and Low-Shelf Filters] → [Cabinet Simulation (IR)] → [Output]

### Подробное описание блоков:

| Блок | Описание |
|------|----------|
| Input | Входной сигнал |
| High-Pass and Low-Pass Filters | Убирает низкие и верхние частоты до усиления |
| Tube Clipping + Interstage Filters | Имитация перегрузки лампового усилителя с межкаскадными фильтрами |
| Tone Control | Регулировка тембра через High-Shelf фильтр |
| Mid-Cut and Low-Shelf Filters | Добавляет низкие и вырезает средние частоты после усиления для более плотного звука |
| Cabinet Simulation (IR) | Свёртка с импульсной характеристикой гитарного динамика |
| Output | Выходной сигнал |

---

## Технологии

- **JUCE Framework** – UI и работа с аудио
- **CMake** – система сборки

---

## Установка и сборка

```bash
# Клонирование репозитория
git clone https://github.com/KeHT-B-baGazhHNke/JuceAudioPluginTemplate-main.git 
cd JuceAudioPluginTemplate-main

# Сборка
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```
- После сборки плагин будет находиться в директории build/Guitar_Amp_Emulation_artefacts/Release.
