import {
    Grid,
    GridItem,
    Tabs,
    TabList,
    TabPanels,
    Tab,
    TabPanel,
    Text,
    Box,
    ListItem,
    ListIcon,
    List,
} from '@chakra-ui/react'

import { Image } from '@chakra-ui/react'
import topot16 from '../assets/top_nobg.webp'
import { MdCircle } from 'react-icons/md'

export default function QuickStart() {
    return (
        <Grid templateColumns="repeat(5, 1fr)" p={5} px="5vw">
            <GridItem colSpan={2} pr={3}>
                <Box width="90%" mx="auto" display="flex" alignItems="center">
                    <Image src={topot16} alt="Topo T16" />
                </Box>
            </GridItem>
            <GridItem colSpan={3} px={5}>
                <Tabs>
                    <TabList>
                        <Tab>Intro</Tab>
                        <Tab>Interface</Tab>
                        <Tab>Keyboard Mode</Tab>
                        <Tab>Joystick Mode</Tab>
                        <Tab>Fader Mode</Tab>
                    </TabList>

                    <TabPanels>
                        <TabPanel>
                            <Text fontSize="large">
                                The Topo T16 is a compact expressive Midi
                                controller. It's equipped with 16
                                pressure-sensitive keys, that output velocity
                                and aftertouch. The touchstrip is used to
                                control useful parameters, like pitch bend,
                                octave and more. The T16 is designed to be used
                                with a computer via the USB C port or any DIN
                                MIDI device, using the MIDI trs port. The Device
                                can do way more than just sending MIDI notes and
                                can be configured to your needs using the simple
                                and intuitive web interface.
                            </Text>
                        </TabPanel>
                        <TabPanel>
                            <Text fontSize="large">
                                Lorem ipsum dolor sit amet consectetur
                                adipisicing elit. Similique tempore totam ipsum
                                sunt adipisci natus perferendis pariatur
                                quisquam sint eos quis illo exercitationem
                                debitis, rem dolor dignissimos provident
                                accusantium accusamus.
                            </Text>
                        </TabPanel>
                        <TabPanel>
                            <Text fontSize="large">
                                When in Keyboard mode, the T16 will output Midi
                                NoteOn/Off messages. Both the velocity and the
                                aftertouch can be configured with your choice of
                                reaction curve. Also the layout is fully
                                customizable, allowing you set a scale
                                (chromatic by default), the root note and the
                                "direction" to fit your playing style. To help
                                you navigate the scale, root notes are
                                highlighted on the grid using the Key's RGB
                                leds. In this mode the touchstrip has the
                                following functions:
                            </Text>
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
                                    <ListIcon
                                        as={MdCircle}
                                        color="orange.400"
                                    />
                                    Mod Wheel
                                </ListItem>
                                <ListItem>
                                    <ListIcon as={MdCircle} color="red.400" />
                                    Bank Select
                                </ListItem>
                            </List>
                        </TabPanel>
                        <TabPanel>
                            <Text fontSize="large">
                                In Joystick mode, the T16 acts as a X-Y (and Z)
                                controller, sending CC messages based on the
                                position of a virtual cursor on the grid. In
                                this mode the touchstrip sets the slew rate of
                                the cursor. By pressing a key the cursor will
                                move torwards it, the harder you press the
                                faster it will move in regard to the set Slew
                                rate. Pushing on more than one key will move the
                                cursor to the average position of the pressed
                                keys. The pressure is sent as the Z value. It
                                can be configured to snap to the center to
                                simulate a spring-loaded joystick.
                            </Text>
                        </TabPanel>
                        <TabPanel>
                            <Text fontSize="large">
                                In Fader mode, the T16 acts as a 4 channel fader
                                bank, with each vertical column of keys acting
                                as a single fader. Like in the Pad mode, the
                                touchstrip sets the slew rate of the fader.
                            </Text>
                        </TabPanel>
                    </TabPanels>
                </Tabs>
            </GridItem>
        </Grid>
    )
}
