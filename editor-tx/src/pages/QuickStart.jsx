import {
    Grid,
    GridItem,
    Text,
    Box,
    ListItem,
    ListIcon,
    List,
    Heading,
    Table,
    TableContainer,
    Thead,
    Th,
    Tr,
    Tbody,
    Td,
    Flex,
    HStack,
    Button,
    Spacer,
    VStack,
    Link,
} from '@chakra-ui/react'

import { Image } from '@chakra-ui/react'
import Interface from '../assets/interface.webp'
import KeyboardLayout from '../assets/keyboard_layout.webp'
import JoystickLayout from '../assets/joystick.webp'
import FadersLayout from '../assets/faders.webp'
import StrumLayout from '../assets/strum_layout.webp'
import QuickSettingsLayout from '../assets/quicksettings_layout.webp'
import { MdCircle, MdModeEdit } from 'react-icons/md'
import React, { useEffect, useRef, useState } from 'react'

const content = [
    {
        title: 'Intro',
        sections: [
            {
                content: (
                    <>
                        <strong>Topo T16</strong> is a compact expressive Midi
                        controller. It&apos;s equipped with 16
                        <strong> pressure-sensitive </strong>
                        keys, capable of generating velocity and aftertouch
                        messages. The touchstrip can be used to control useful
                        parameters, like pitch bend, octave. The T16 is designed
                        to be used with a computer via the
                        <strong> USB C</strong> port, any hardware Midi device,
                        using the <strong>MIDI trs</strong> port, or using{' '}
                        <strong>BLE</strong> to wirelessly connect to Android
                        and iOS devices. Topo can do way more than just sending
                        MIDI notes and can be configured to your needs using the
                        simple and intuitive web interface.
                    </>
                ),
            },
            {
                subtitle: 'Ready to go',
                content:
                    "You don't need to install or do anything to start using Topo with your computer. Simply power it via the Usb C port and it will appear as 'Topo T16' in your computer's device list. Check your DAW's manual for instructions on how to connect and use MIDI devices.",
            },
            {
                subtitle: 'The Web Editor',
                content:
                    "The Web Editor allows you to customize many of Topo's feature, like the keyboard layout or the CC and Channel mappings. Simply go to topo.unwn.dev and connect your device. Note: the editor uses WebMidi to talk with your device, so you'll need a compatible browser, like Chrome or Firefox(using the WebMidi extension).",
            },
            {
                subtitle: 'Banks',
                content:
                    "The T16 has 4 banks, each with its own settings and mappings, and can be customized using the editor (or the quicksetting menu, see the dedicated section for more info). To navigate between banks, press the 'Slider button' until the Touch Slider leds turn pink, then use the slider to select the bank. New banks gets loaded instantly, this is great when you use the same mapping on every bank (again, more on this later).",
            },
        ],
    },
    {
        title: 'Interface',
        sections: [
            {
                content: `Topo has a minimal interface, consisting of a 4x4 pressure-sensitive key grid, a capacitive touch slider and two buttons.
                the buttons are used to navigate between the various modes of the T16, as well as trigger alternate functions.`,
                image: Interface,
            },
        ],
    },
    {
        title: 'Keyboard',
        sections: [
            {
                content: (
                    <>
                        When in Keyboard mode, the T16 will output Midi
                        NoteOn/Off messages. Both the velocity and the
                        aftertouch can be configured with your choice of
                        reaction curve. The layout is fully customizable,
                        allowing you to set a scale (chromatic by default), the
                        root note and the direction to fit your playing style.
                        To help you navigate the scale, root notes are
                        highlighted on the grid using the Key's RGB leds. In
                        this mode the touchstrip has the following functions:
                        <List>
                            <ListItem>
                                <ListIcon as={MdCircle} color="pink.400" />
                                Pitch Bend
                            </ListItem>
                            <ListItem>
                                <ListIcon as={MdCircle} color="blue.300" />
                                Octave Switch
                            </ListItem>
                            <ListItem>
                                <ListIcon as={MdCircle} color="orange.400" />
                                Mod Wheel
                            </ListItem>
                            <ListItem>
                                <ListIcon as={MdCircle} color="red.400" />
                                Bank Select
                            </ListItem>
                        </List>
                    </>
                ),
                image: KeyboardLayout,
                caption:
                    'The keyboard leds highlight each root note for the selected scale',
            },
        ],
    },
    {
        title: 'Strum',
        sections: [
            {
                content: `Strum mode transforms the T16 in a stringed instrument, perfect for playing arpeggios or plucked sounds.
                In this mode the Key Grid is used to select the note and the chord to play, while the touchstrip is used as virtual strings.
                The first 12 keys are used to select the note (picked from the current scale), while the bottom 4 keys are used to select the chord quality, Major, Minor, Augumented and Diminished. If a scale other than the chromatic is selected the default chords are already set based on scale grades.`,
                image: StrumLayout,
            },
        ],
    },
    {
        title: 'Joystick',
        sections: [
            {
                content: `In Joystick mode, the T16 acts as a X-Y (and Z) controller, sending CC messages based on
                          the position of a virtual cursor on the grid. In this mode the touchstrip sets the
                          slew rate of the cursor. By pressing a key the cursor will move towards it, the harder
                          you press the faster it will move in regard to the set Slew rate. Pushing on more than
                          one key will move the cursor to the average position of the pressed keys. The pressure
                          is sent as the Z value. It can be configured to snap to the center to simulate a
                          spring-loaded joystick.`,
                image: JoystickLayout,
            },
        ],
    },
    {
        title: 'Faders',
        sections: [
            {
                content: `In Fader mode, the T16 acts as a 4 channel
                                    fader bank, with each vertical column of
                                    keys acting as a single fader. Like for
                                    Joystick mode, the touchstrip sets the slew
                                    rate of the fader. `,
                image: FadersLayout,
            },
        ],
    },
    {
        title: 'Quick Settings',
        sections: [
            {
                content: `The quick settings menu can be accessed by
                          pressing both Slider and Mode buttons at the
                          same time. The grid leds will quickly flash
                          in a sequence and the quick settings menu
                          will open. There are 3 pages, each with
                          their own set of 4 settings. Use the slider
                          to navigate between pages, the top 4 keys
                          select the setting and the bottom 12 are used
                          for the value. the key leds will light up
                          accordingly to the amount of values
                          available.`,
            },
            {
                content: (
                    <TableContainer mt={3}>
                        <Table size="sm">
                            <Thead>
                                <Tr>
                                    <Th>Page</Th>
                                    <Th>Index</Th>
                                    <Th>Setting</Th>
                                    <Th isNumeric>Range</Th>
                                </Tr>
                            </Thead>
                            <Tbody>
                                <Tr>
                                    <Td>1</Td>
                                    <Td>1</Td>
                                    <Td>Brightness</Td>
                                    <Td isNumeric>1-8</Td>
                                </Tr>
                                <Tr>
                                    <Td></Td>
                                    <Td>2</Td>
                                    <Td>Enable Trs</Td>
                                    <Td isNumeric>Off/On</Td>
                                </Tr>
                                <Tr>
                                    <Td></Td>
                                    <Td>3</Td>
                                    <Td>TRS Type</Td>
                                    <Td isNumeric>A/B</Td>
                                </Tr>
                                <Tr>
                                    <Td></Td>
                                    <Td>4</Td>
                                    <Td>Midi BLE</Td>
                                    <Td isNumeric>Off/On</Td>
                                </Tr>
                                <Tr>
                                    <Td>2</Td>
                                    <Td>1</Td>
                                    <Td>Channel</Td>
                                    <Td isNumeric>1-12</Td>
                                </Tr>
                                <Tr>
                                    <Td></Td>
                                    <Td>2</Td>
                                    <Td>Scale</Td>
                                    <Td isNumeric>1-12</Td>
                                </Tr>
                                <Tr>
                                    <Td></Td>
                                    <Td>3</Td>
                                    <Td>Octave</Td>
                                    <Td isNumeric>1-8</Td>
                                </Tr>
                                <Tr>
                                    <Td></Td>
                                    <Td>4</Td>
                                    <Td>Root Note</Td>
                                    <Td isNumeric>C-B</Td>
                                </Tr>
                                <Tr>
                                    <Td>3</Td>
                                    <Td>1</Td>
                                    <Td>Velocity Curve</Td>
                                    <Td isNumeric>1-4</Td>
                                </Tr>
                                <Tr>
                                    <Td></Td>
                                    <Td>2</Td>
                                    <Td>Aftertouch Curve</Td>
                                    <Td isNumeric>1-4</Td>
                                </Tr>
                                <Tr>
                                    <Td></Td>
                                    <Td>3</Td>
                                    <Td>Flip X</Td>
                                    <Td isNumeric>false/true</Td>
                                </Tr>
                                <Tr>
                                    <Td></Td>
                                    <Td>4</Td>
                                    <Td>Flip Y</Td>
                                    <Td isNumeric>false/true</Td>
                                </Tr>
                            </Tbody>
                        </Table>
                    </TableContainer>
                ),
                image: QuickSettingsLayout,
            },
        ],
    },
]

export default function QuickStart() {
    const sectionRefs = useRef(content.map(() => React.createRef()))
    const [activeSection, setActiveSection] = useState(0)

    const scrollToSection = (index) => {
        const offsetTop = sectionRefs.current[index].current.offsetTop - 60 // Adjust for navbar height
        window.scrollTo({ top: offsetTop, behavior: 'smooth' })
        setActiveSection(index)
    }

    useEffect(() => {
        const handleScroll = () => {
            const scrollPosition = window.scrollY + 70 // Adjust for navbar height
            sectionRefs.current.forEach((ref, index) => {
                const sectionTop = ref.current.offsetTop
                const sectionHeight = ref.current.offsetHeight
                if (
                    scrollPosition >= sectionTop &&
                    scrollPosition < sectionTop + sectionHeight
                ) {
                    setActiveSection(index)
                }
            })
        }

        window.addEventListener('scroll', handleScroll)
        return () => {
            window.removeEventListener('scroll', handleScroll)
        }
    }, [])

    return (
        <>
            <Flex
                as="nav"
                p={2}
                px={6}
                alignItems="center"
                position="sticky"
                top={0}
                zIndex={100}
                bg="#ffffffcf"
            >
                <HStack spacing={4}>
                    <Heading as="h1" size="lg" letterSpacing={1}>
                        Topo T16 Manual
                    </Heading>
                    <Button
                        as="a"
                        href="/"
                        rightIcon={<MdModeEdit />}
                        colorScheme="primary"
                        variant="outline"
                        target="_blank"
                    >
                        Editor
                    </Button>
                </HStack>
                <Spacer />
            </Flex>
            <Grid templateColumns="repeat(5, 1fr)" p={5} px="5vw">
                <GridItem colSpan={1} pr={3}>
                    <Box
                        position="fixed"
                        top="100px"
                        width="90%"
                        mx="auto"
                        display="flex"
                        flexDirection="column"
                        alignItems="flex-start"
                    >
                        <VStack align="start" spacing={2}>
                            {content.map((section, index) => (
                                <Link
                                    key={index}
                                    onClick={() => scrollToSection(index)}
                                    cursor="pointer"
                                    fontWeight={
                                        activeSection === index
                                            ? 'bold'
                                            : 'normal'
                                    }
                                >
                                    {section.title}
                                </Link>
                            ))}
                        </VStack>
                    </Box>
                </GridItem>
                <GridItem colSpan={4} px={5}>
                    <Box>
                        {content.map((section, sectionIndex) => (
                            <Box
                                key={sectionIndex}
                                mb={6}
                                ref={sectionRefs.current[sectionIndex]}
                            >
                                <Heading size="lg" mb={4}>
                                    {section.title}
                                </Heading>
                                {section.sections.map((subSection, index) => (
                                    <Box key={index} mb={4}>
                                        {subSection.subtitle && (
                                            <Heading size="md" mt={3}>
                                                {subSection.subtitle}
                                            </Heading>
                                        )}
                                        {subSection.image ? (
                                            <Grid
                                                templateColumns="repeat(2, 1fr)"
                                                gap={4}
                                            >
                                                <GridItem>
                                                    <Text fontSize="large">
                                                        {subSection.content}
                                                    </Text>
                                                </GridItem>
                                                <GridItem>
                                                    <VStack justifyContent="center">
                                                        <Image
                                                            src={
                                                                subSection.image
                                                            }
                                                            alt={
                                                                subSection.subtitle ||
                                                                section.title
                                                            }
                                                            w="80%"
                                                        />
                                                        {subSection.caption && (
                                                            <Text fontSize="small">
                                                                {
                                                                    subSection.caption
                                                                }
                                                            </Text>
                                                        )}
                                                    </VStack>
                                                </GridItem>
                                            </Grid>
                                        ) : (
                                            <Text fontSize="large">
                                                {subSection.content}
                                            </Text>
                                        )}
                                    </Box>
                                ))}
                            </Box>
                        ))}
                    </Box>
                </GridItem>
            </Grid>
        </>
    )
}
