import { Box, Flex, IconButton, Spacer } from '@chakra-ui/react'
import { Link } from 'react-router-dom'
import { FaEdit, FaUpload, FaBook } from 'react-icons/fa'

const AppNav = () => {
    return (
        <Box bg="gray.100" py={2}>
            <Flex maxW="container.lg" mx="auto" alignItems="center">
                <IconButton
                    as={Link}
                    to="/"
                    icon={<FaEdit />}
                    aria-label="Editor"
                    variant="ghost"
                    mr={2}
                />
                <IconButton
                    as={Link}
                    to="/upload"
                    icon={<FaUpload />}
                    aria-label="Upload"
                    variant="ghost"
                    mr={2}
                />
                <IconButton
                    as={Link}
                    to="/manual"
                    icon={<FaBook />}
                    aria-label="Manual"
                    variant="ghost"
                />
                <Spacer />
            </Flex>
        </Box>
    )
}

export default AppNav
