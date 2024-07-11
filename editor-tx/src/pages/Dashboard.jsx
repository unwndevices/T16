import {
    Box,
    Grid,
    GridItem,
    Tabs,
    TabList,
    TabPanels,
    Tab,
    TabPanel,
} from '@chakra-ui/react'
import ControlChange from './ControlChange'
import Keyboard from './Keyboard'
import Scales from './Scales'
import Settings from './Settings'
import Footer from '../components/Footer'

export default function Dashboard() {
    return (
        <>
            <Grid templateColumns="repeat(4, 1fr)" p={5} px="5vw" mb={10}>
                <GridItem colSpan={4}>
                    <Tabs variant="enclosed" colorScheme="blue">
                        <TabList>
                            <Tab fontSize="md" letterSpacing={1}>
                                Keyboard
                            </Tab>
                            <Tab fontSize="md" letterSpacing={1}>
                                Custom Scales
                            </Tab>
                            <Tab fontSize="md" letterSpacing={1}>
                                CC Mapping
                            </Tab>
                            <Tab fontSize="md" letterSpacing={1}>
                                Global Settings
                            </Tab>
                        </TabList>
                        <TabPanels>
                            <TabPanel>
                                <Keyboard />
                            </TabPanel>
                            <TabPanel>
                                <Scales />
                            </TabPanel>
                            <TabPanel>
                                <ControlChange />
                            </TabPanel>
                            <TabPanel>
                                <Settings />
                            </TabPanel>
                        </TabPanels>
                    </Tabs>
                </GridItem>
            </Grid>

            <Footer />
        </>
    )
}
