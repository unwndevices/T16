import { useState, useEffect, useRef, createRef, type RefObject } from 'react'
import { Card } from '@/design-system'
import { MdCircle, MdOutlineSystemUpdateAlt } from 'react-icons/md'
import InterfaceImg from '@/assets/Interface.webp'
import KeyboardLayoutImg from '@/assets/keyboard_layout.webp'
import JoystickImg from '@/assets/joystick.webp'
import FadersImg from '@/assets/faders.webp'
import StrumImg from '@/assets/strum_layout.webp'
import QuickSettingsImg from '@/assets/quicksettings_layout.webp'
import styles from './Manual.module.css'

interface SubSection {
  subtitle?: string
  content: React.ReactNode
  image?: string
  caption?: string
}

interface Section {
  title: string
  sections: SubSection[]
}

const content: Section[] = [
  {
    title: 'Intro',
    sections: [
      {
        content: (
          <>
            <strong>Topo T16</strong> is a compact expressive MIDI controller.
            It is equipped with 16 <strong>pressure-sensitive</strong> keys,
            capable of generating velocity and aftertouch messages. The
            touchstrip can be used to control useful parameters, like pitch bend
            and octave. The T16 is designed to be used with a computer via the{' '}
            <strong>USB C</strong> port, any hardware MIDI device using the{' '}
            <strong>MIDI TRS</strong> port, or using <strong>BLE</strong> to
            wirelessly connect to Android and iOS devices. Topo can do way more
            than just sending MIDI notes and can be configured to your needs
            using the simple and intuitive web interface.
          </>
        ),
      },
      {
        subtitle: 'Ready to go',
        content:
          "You don't need to install or do anything to start using Topo with your computer. Simply power it via the USB C port and it will appear as 'Topo T16' in your computer's device list. Check your DAW's manual for instructions on how to connect and use MIDI devices.",
      },
      {
        subtitle: 'The Web Editor',
        content: (
          <>
            The Web Editor allows you to customize many of Topo's features, like
            the keyboard layout or the CC and Channel mappings. Simply go to
            topo.unwn.dev and connect your device. Note: the editor uses WebMIDI
            to talk with your device, so you'll need to use Chrome or
            Chrome-based browsers (there is a WebMIDI extension for Firefox, but
            it is not reliable enough).
          </>
        ),
      },
      {
        subtitle: 'Banks',
        content:
          'The T16 has 4 banks, each with its own settings and mappings, and can be customized using the editor (or the quick settings menu, see the dedicated section for more info). To navigate between banks, press the "Slider button" until the Touch Slider LEDs turn pink, then use the slider to select the bank. New banks get loaded instantly, which is great when you use the same mapping on every bank.',
      },
    ],
  },
  {
    title: 'Interface',
    sections: [
      {
        content:
          'Topo has a minimal interface, consisting of a 4x4 pressure-sensitive key grid, a capacitive touch slider and two buttons. The buttons are used to navigate between the various modes of the T16, as well as trigger alternate functions.',
        image: InterfaceImg,
      },
    ],
  },
  {
    title: 'Keyboard',
    sections: [
      {
        content: (
          <>
            When in Keyboard mode, the T16 will output MIDI NoteOn/Off messages.
            Both the velocity and the aftertouch can be configured with your
            choice of reaction curve. The layout is fully customizable, allowing
            you to set a scale (chromatic by default), the root note and the
            direction to fit your playing style. To help you navigate the scale,
            root notes are highlighted on the grid using the key's RGB LEDs. In
            this mode the touchstrip has the following functions:
            <ul className={styles.sliderModes}>
              <li>
                <MdCircle className={styles.dotRed} /> Pitch Bend
              </li>
              <li>
                <MdCircle className={styles.dotBlue} /> Octave Switch
              </li>
              <li>
                <MdCircle className={styles.dotGreen} /> Mod Wheel
              </li>
              <li>
                <MdCircle className={styles.dotPink} /> Bank Select
              </li>
            </ul>
          </>
        ),
        image: KeyboardLayoutImg,
        caption:
          'The keyboard LEDs highlight each root note for the selected scale',
      },
      {
        content:
          'NoteOn/Off sensitivity (threshold) can be configured in the Web Editor, under general settings.',
      },
    ],
  },
  {
    title: 'Strum',
    sections: [
      {
        content:
          'Strum mode transforms the T16 into a stringed instrument, perfect for playing arpeggios or plucked sounds. In this mode the Key Grid is used to select the note and the chord to play, while the touchstrip is used as virtual strings. The first 12 keys are used to select the note (picked from the current scale), while the bottom 4 keys are used to select the chord quality: Major, Minor, Augmented, and Diminished. If a scale other than the chromatic is selected, the default chords are already set based on scale grades.',
        image: StrumImg,
      },
    ],
  },
  {
    title: 'Joystick',
    sections: [
      {
        content:
          'In Joystick mode, the T16 acts as an X-Y (and Z) controller, sending CC messages based on the position of a virtual cursor on the grid. In this mode the touchstrip sets the slew rate of the cursor. By pressing a key the cursor will move towards it; the harder you press the faster it will move in regard to the set slew rate. Pushing on more than one key will move the cursor to the average position of the pressed keys. The pressure is sent as the Z value.',
        image: JoystickImg,
      },
    ],
  },
  {
    title: 'Faders',
    sections: [
      {
        content:
          'In Fader mode, the T16 acts as a 4-channel fader bank, with each vertical column of keys acting as a single fader. Like for Joystick mode, the touchstrip sets the slew rate of the fader.',
        image: FadersImg,
      },
    ],
  },
  {
    title: 'Quick Settings',
    sections: [
      {
        content:
          'The quick settings menu can be accessed by pressing both Slider and Mode buttons at the same time. The grid LEDs will quickly flash in a sequence and the quick settings menu will open. There are 3 pages, each with their own set of 4 settings. Use the slider to navigate between pages, the top 4 keys select the setting and the bottom 12 are used for the value. The key LEDs will light up accordingly to the amount of values available.',
      },
      {
        content: (
          <div className={styles.tableWrapper}>
            <table className={styles.settingsTable}>
              <thead>
                <tr>
                  <th>Page</th>
                  <th>Index</th>
                  <th>Setting</th>
                  <th>Range</th>
                </tr>
              </thead>
              <tbody>
                <tr><td>1</td><td>1</td><td>Brightness</td><td>1-8</td></tr>
                <tr><td></td><td>2</td><td>Enable TRS</td><td>Off/On</td></tr>
                <tr><td></td><td>3</td><td>TRS Type</td><td>A/B</td></tr>
                <tr><td></td><td>4</td><td>MIDI BLE</td><td>Off/On</td></tr>
                <tr><td>2</td><td>1</td><td>Channel</td><td>1-12</td></tr>
                <tr><td></td><td>2</td><td>Scale</td><td>1-12</td></tr>
                <tr><td></td><td>3</td><td>Octave</td><td>1-8</td></tr>
                <tr><td></td><td>4</td><td>Root Note</td><td>C-B</td></tr>
                <tr><td>3</td><td>1</td><td>Velocity Curve</td><td>1-4</td></tr>
                <tr><td></td><td>2</td><td>Aftertouch Curve</td><td>1-4</td></tr>
                <tr><td></td><td>3</td><td>Flip X</td><td>false/true</td></tr>
                <tr><td></td><td>4</td><td>Flip Y</td><td>false/true</td></tr>
              </tbody>
            </table>
          </div>
        ),
        image: QuickSettingsImg,
      },
    ],
  },
  {
    title: 'Reset, Backup and Update',
    sections: [
      {
        subtitle: 'Reset',
        content:
          'To reset the configuration to factory settings, hold the SLIDER MODE button while plugging the device in. The key grid will flash in a sequence. Note that the current configuration is lost -- remember to backup your settings.',
      },
      {
        subtitle: 'Backup and Restore',
        content:
          'The web editor can backup and restore configuration files (with *.topo extension, JSON format). Use the buttons in the footer to backup and restore your configuration. The configuration file is human-readable and can be edited manually.',
      },
      {
        subtitle: 'Update',
        content: (
          <>
            To update the firmware of the device you can use Topo's update tool
            (accessible on the header, look for this icon:{' '}
            <MdOutlineSystemUpdateAlt className={styles.inlineIcon} />
            ), simply select the firmware version from the dropdown menu and
            follow the on-screen instructions.
          </>
        ),
      },
    ],
  },
]

export function Manual() {
  const sectionRefs = useRef<RefObject<HTMLDivElement | null>[]>(
    content.map(() => createRef<HTMLDivElement>())
  )
  const [activeSection, setActiveSection] = useState(0)

  const scrollToSection = (index: number) => {
    const el = sectionRefs.current[index].current
    if (!el) return
    const offsetTop = el.offsetTop - 60
    window.scrollTo({ top: offsetTop, behavior: 'smooth' })
    setActiveSection(index)
  }

  useEffect(() => {
    const handleScroll = () => {
      const scrollPosition = window.scrollY + 70
      sectionRefs.current.forEach((ref, index) => {
        const el = ref.current
        if (!el) return
        const sectionTop = el.offsetTop
        const sectionHeight = el.offsetHeight
        if (
          scrollPosition >= sectionTop &&
          scrollPosition < sectionTop + sectionHeight
        ) {
          setActiveSection(index)
        }
      })
    }

    window.addEventListener('scroll', handleScroll)
    return () => window.removeEventListener('scroll', handleScroll)
  }, [])

  return (
    <div className={styles.page}>
      <aside className={styles.sidebar}>
        <nav className={styles.nav}>
          {content.map((section, index) => (
            <button
              key={index}
              className={`${styles.navLink} ${activeSection === index ? styles.navLinkActive : ''}`}
              onClick={() => scrollToSection(index)}
            >
              {section.title}
            </button>
          ))}
        </nav>
      </aside>

      <main className={styles.content}>
        {content.map((section, sectionIndex) => (
          <div
            key={sectionIndex}
            className={styles.section}
            ref={sectionRefs.current[sectionIndex]}
          >
            <h2 className={styles.sectionTitle}>{section.title}</h2>
            {section.sections.map((sub, subIndex) => (
              <div key={subIndex} className={styles.subSection}>
                {sub.subtitle && (
                  <h3 className={styles.subSectionTitle}>{sub.subtitle}</h3>
                )}
                {sub.image ? (
                  <div className={styles.imageRow}>
                    <div className={styles.imageRowText}>
                      {typeof sub.content === 'string' ? (
                        <p>{sub.content}</p>
                      ) : (
                        sub.content
                      )}
                    </div>
                    <div className={styles.imageRowMedia}>
                      <img
                        src={sub.image}
                        alt={sub.subtitle ?? section.title}
                        className={styles.image}
                      />
                      {sub.caption && (
                        <span className={styles.caption}>{sub.caption}</span>
                      )}
                    </div>
                  </div>
                ) : typeof sub.content === 'string' ? (
                  <p className={styles.bodyText}>{sub.content}</p>
                ) : (
                  <div className={styles.bodyText}>{sub.content}</div>
                )}
              </div>
            ))}
          </div>
        ))}
      </main>
    </div>
  )
}
