import { useContext } from 'react'
import { Skeleton } from '@chakra-ui/react'
import MidiContext from './MidiProvider'
import { PropTypes } from 'prop-types'

export default function SkeletonLoader({ children }) {
    const { isConnected } = useContext(MidiContext)

    return (
        <Skeleton borderRadius="md" isLoaded={isConnected}>
            {children}
        </Skeleton>
    )
}

SkeletonLoader.propTypes = {
    children: PropTypes.node,
}
