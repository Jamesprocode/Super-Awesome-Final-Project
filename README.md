# Software Design Document: Super Awesome Final Project (MUSI 6106)

**Author(s):** Angela Branchek, Rafael Collado, Binyue Deng, JD Harris, Jiayi Wang

## Canvas assignment bullet points
### motivation, problem to be solved, why is there a need for this
  - Motivations
    - Existing production workflows require repeatedly assembling and managing multi-plugin processing chains, leading to unnecessary setup time. There is a need for a unified, modular environment that preserves professional control and transparency while significantly reducing interaction overhead and enabling faster iteration through a single interface.
    - Many users lack the ability to design effective processing chains, while existing “simplified” tools obscure underlying structure and limit growth. There is a need for an abstraction layer—via macro controls and also a choice for the deep multi-parameter conrol. Such a system can serve not only as a tool, but as a platform for users to design, customize, and share processing structures and workflows across skill levels.
### applications, use cases, target users, context, environment
  - Primary applications
    - Vocal mixing: pop/rap/EDM vocals, podcasts/voiceover, streaming.
    - Instrument chains: guitar DI, bass, synth leads, drum bus “quick polish”.
  - Use case
    - Target User 1 (Experience or professional engineers)
      - Reduce repetitive setup work while preserving full control, transparency, and professional gain-staging practices.
      - Enable experienced engineers to instantiate a complete processing chain from a single plugin instance, eliminating the need to load, manage, and navigate multiple discrete plugins.
      - Allow user to create and save their chain and map to macro control for future use or sharing with others
      - Outcome: Engineers spend less time on setup and window management, and more time on critical listening and decision-making.
    - Target User 2 (Beginer Users)
      - Lower the barrier to chain processing through presets.
      - Abstract complex multi-parameter interactions into a small number of intuitive macro controls
  - Context / Environment
    - DAWs: Abelton, Logic, etc
    - Formats: VST3 or AU (maybe AAX? depends on time)
    - Performance constraints: real-time processing
    - Typical workflow: insert on vocal track(s), save chain preset per vocalist/song, recall quickly
### functionality from user point of view and how it differentiates from similar products
  - Use Case (Target User)
    - For experienced engineers
      - “I always insert EQ → Comp → De-esser → Verb/Delay. Saving time in one plug-in instead of instantiate 5 plugins every time.”
      - “I want to tweak my whole chain from one UI without opening/closing plugin windows.”
      - “I want per-module bypass/order changes without losing my gain staging.”
    - For beginners
      -  “Give me a good starting point (‘EDM Vocal’, ‘Podcast Clean’, ‘Rap Presence’)”
      -  “Let me drive the sound with 2 macro knobs (e.g., Clean ↔ Aggressive, Dry ↔ Wet)”
      -  “I can dive deeper later, but I want it to sound decent immediately.”
  - Difference from similar product
    - Curent Similar product (UA Topline Vocal Suite, Effect Rack, XVOX Pro)
      - fixed order or limited reordering
      - fixed modules
      - “macro” controls are hardcoded per preset (not user-mappable)
    - Our Difference
      - True modular chain: add/remove/reorder modules easily.
      - Macro mapping system that’s user-editable:
      - Two-layer UX
        - Macro "Two Knob" that mapps to all the parameters
        - Pro view of full paramter control and design presets
### plans for implementation: flow chart, processing blocks, needed components, potential need for 3rd party libs
  - JUCE
### algorithmic references - which reference do you base your algorithmic implementations on?
  - JUCE AudioProcessorValueTreeState
### general responsibilities and work assignments (can overlap)
  - Each group member work on one of the processing chain (eq, compressor, de-esser, reverb, delay, autotune?, something to choose from)
  - decide how to break up the work for UI
### Timeline
  - Feburary : Download JUCE and learn the documentation and talk about the data structure and the signal flow
  - March: Each of the group member work on their own processing DSP components
  - April assemble the DSP components and work on the UI design with the big knobs implementation

---

## Filled-in Version
- We want to take the guesswork out of long plugin chains for [INSERT HERE: vocals, guitar, etc??] and provide producers a sleek, straightforward plugin with only one or two knobs to effortlessly improve a track's sound. By orchestrating multiple effects in tandem with a single knob, we can help users get to a sound they love faster and easier.
- This plugin is useful for anyone, from novice producers looking for a fool-proof way to level up their sound, to commerical audio engineers who want a quick and reliable way to get their work done without sacrificing quality.
- Our plugin will do the heavy lifting in the background for the user, making sure the interface is as fool-proof as possible.
- We will be developing this product as a VST plugin. We will have to research and iterate on the underlying chain and how our knob(s) affect the sound.
- We are partially inspired by plugins like Fresh Air and the OneKnob series, who offer minimal user interfaces with impressive output quality.
- general responsibilities and work assignments (can overlap) [INSERT HERE]
- (Bonus) If you can create a timeline and make use of the Github Projects functionality [i don't care about this tbh]

---
## PREVIOUS README (DELETE AFTER ABOVE IS COMPLETE?)
## 1. Summary
The production process of writing music in a DAW can be overwhelming and sometimes frustrating when you have so many options and settings that can be tweaked. Some producers have preferred plugin chains that they have assembled from years of experience, but even with that comes the need for slight adjustments depending on the nature of the audio source, which can waste time during a session. Other producers that are less experienced might struggle with figuring out where to start when several plugins with even more parameters to edit are given to them. If there was a way to reduce the headache of needing to dial in the perfect sound without limiting the user, both beginner and experienced music producers would benefit.

Our final project involves creating a plugin with several pre-programmed effects that can be adjusted and ordered to create a chain on a vocal or instrument track. However, there are many other plugins that can already do this, so the feature that would make this plugin unique would allow users to interact with one or two knobs that would control parameters from multiple effects simultaneously. This would not only allow users to get a desirable sound in a much more straight-forward interface, but would keep the customization abilities of adjusting multiple effects parameters by allowing users to create their own macro knobs.

We would create this plugin with JUCE, an audio plugin development framework that will make allow us to handle the DSP and interface design more easily. With JUCE, we plan to distribute this plugin in the VST3 and AU formats, with the potential to support AAX as well.

## 2. Plan
### 2.1 Goals
* Process audio in real-time
* Program several effects: EQ, compression, reverb, delay, chorus, distortion
* Create several preset macro knobs
* Allow users to make their own macro knobs

### 2.2 Non-Goals
* Supporting external plugins

### 2.3 Success Metrics
* Minimal latency (under 10ms)
* Will have most commonly used effects implemented
* Will be pleasant for users to work with
* Users will not have a hard time or feel limited when dialing in a desired sound
* Users will be able to find a desired sound more quickly

---

## 3. Technical Architecture 
* **JUCE Framework**: 

---

## 4. UI/UX Design
* **Visual Elements:** Layout description
* **Visualizers:** Real-time spectrum analyzer or waveform display?

---

## 5. DSP Implementation Details
Describe any DSP algorithms that will be used
