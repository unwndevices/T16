import {
    Card,
    CardBody,
    CardHeader,
    HStack,
    IconButton,
} from '@chakra-ui/react'
import { PropTypes } from 'prop-types'
import SkeletonLoader from './SkeletonLoader'
import { MdArrowLeft, MdArrowRight } from 'react-icons/md'

export default function KeyCard({ name, output, root }) {
    return (
        <Card borderRadius={20} borderColor="gray.100" borderBottomWidth={4}>
            <CardHeader
                borderTopRadius={20}
                textAlign="left"
                backgroundColor={root ? 'primary.800' : 'primary.400'}
                color="white"
                py={3}
            >
                {name}
            </CardHeader>
            <CardBody textAlign="center" fontWeight="600">
                <SkeletonLoader>{output}</SkeletonLoader>
            </CardBody>
        </Card>
    )
}

export function EditableKeyCard({
    name,
    output,
    root,
    onDecrement,
    onIncrement,
}) {
    return (
        <Card borderRadius={20} borderColor="gray.100" borderBottomWidth={4}>
            <CardHeader
                borderTopRadius={20}
                textAlign="left"
                backgroundColor={root ? 'primary.800' : 'primary.400'}
                color="white"
                py={3}
            >
                {name}
            </CardHeader>
            <CardBody textAlign="center" fontWeight="600">
                <HStack justifyContent="space-between">
                    <IconButton
                        onClick={onDecrement}
                        icon={<MdArrowLeft />}
                        size="lg"
                    />
                    <SkeletonLoader>{output}</SkeletonLoader>
                    <IconButton
                        onClick={onIncrement}
                        icon={<MdArrowRight />}
                        size="lg"
                    />
                </HStack>
            </CardBody>
        </Card>
    )
}

KeyCard.propTypes = {
    name: PropTypes.string,
    output: PropTypes.string,
    active: PropTypes.bool,
    root: PropTypes.bool,
}

EditableKeyCard.propTypes = {
    name: PropTypes.string,
    output: PropTypes.string,
    active: PropTypes.bool,
    root: PropTypes.bool,
    onDecrement: PropTypes.func,
    onIncrement: PropTypes.func,
}
