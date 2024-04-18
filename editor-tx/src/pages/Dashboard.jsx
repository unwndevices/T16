import {
    Accordion,
    AccordionButton,
    AccordionIcon,
    AccordionItem,
    AccordionPanel,
    Box,
    Grid,
    GridItem,
} from '@chakra-ui/react'
import ControlChange from './ControlChange'
import Keyboard from './Keyboard'
import Scales from './Scales'

export default function Dashboard() {
    return (
        <Grid templateColumns="repeat(4, 1fr)" p={5} px="5vw">
            <GridItem colSpan={4}>
                <Accordion defaultIndex={[0]} borderRadius="15px">
                    <AccordionItem>
                        <h2>
                            <AccordionButton>
                                <Box
                                    as="span"
                                    flex="1"
                                    textAlign="left"
                                    fontSize="xl"
                                    letterSpacing={1}
                                >
                                    Keyboard
                                </Box>
                                <AccordionIcon />
                            </AccordionButton>
                        </h2>
                        <AccordionPanel>
                            <Keyboard />
                        </AccordionPanel>
                    </AccordionItem>

                    <AccordionItem>
                        <h2>
                            <AccordionButton>
                                <Box
                                    as="span"
                                    flex="1"
                                    textAlign="left"
                                    fontSize="xl"
                                    letterSpacing={1}
                                >
                                    Custom Scales
                                </Box>
                                <AccordionIcon />
                            </AccordionButton>
                        </h2>
                        <AccordionPanel>
                            <Scales />
                        </AccordionPanel>
                    </AccordionItem>

                    <AccordionItem>
                        <h2>
                            <AccordionButton>
                                <Box
                                    as="span"
                                    flex="1"
                                    textAlign="left"
                                    fontSize="xl"
                                    letterSpacing={1}
                                >
                                    CC Mapping
                                </Box>
                                <AccordionIcon />
                            </AccordionButton>
                        </h2>
                        <AccordionPanel>
                            <ControlChange />
                        </AccordionPanel>
                    </AccordionItem>
                </Accordion>
            </GridItem>
        </Grid>
    )
}
