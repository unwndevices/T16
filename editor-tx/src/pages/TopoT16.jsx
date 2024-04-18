import {
    Box,
    Flex,
    Grid,
    Text,
    Image,
    Button,
    Table,
    Thead,
    Tbody,
    Tr,
    Th,
    Td,
    Link,
} from '@chakra-ui/react'

export default function TopoT16() {
    return (
        <Box bg="#E5E5E5">
            <Flex
                position="sticky"
                top={0}
                justifyContent="space-between"
                p={4}
            >
                <Text>Product Name</Text>
                <Flex>
                    <Link mr={4}>Link 1</Link>
                    <Link mr={4}>Link 2</Link>
                    <Link>Link 3</Link>
                </Flex>
                <Flex>
                    <Text mr={4}>$99.99</Text>
                    <Button>Preorder</Button>
                </Flex>
            </Flex>
            <Image src="beauty-shot.jpg" w="100%" />
            <Grid templateColumns="repeat(6, 1fr)" gap={6} p={4}>
                <Box gridColumn="span 2">
                    <Text fontSize="2xl">Subsection Title</Text>
                </Box>
                <Box gridColumn="span 4">
                    <Text>Lorem ipsum...</Text>
                    <Image src="image.jpg" />
                </Box>
            </Grid>
            <Box p={4}>
                <iframe
                    width="560"
                    height="315"
                    src="https://www.youtube.com/embed/video1"
                    allowfullscreen
                ></iframe>
                <iframe
                    width="560"
                    height="315"
                    src="https://www.youtube.com/embed/video2"
                    allowfullscreen
                ></iframe>
            </Box>
            <Box p={4}>
                <Table variant="simple">
                    <Thead>
                        <Tr>
                            <Th>Spec Title</Th>
                            <Th>Feature</Th>
                        </Tr>
                    </Thead>
                    <Tbody>
                        <Tr>
                            <Td>Spec 1</Td>
                            <Td>Feature 1</Td>
                        </Tr>
                    </Tbody>
                </Table>
            </Box>
            <Box p={4}>
                <Image src="big-image.jpg" />
                <Flex justifyContent="center" mt={4}>
                    <Text mr={4}>$99.99</Text>
                    <Button>Buy Now</Button>
                </Flex>
            </Box>
        </Box>
    )
}
