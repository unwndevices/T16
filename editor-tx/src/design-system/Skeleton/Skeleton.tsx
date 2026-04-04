import styles from './Skeleton.module.css'

interface SkeletonProps {
  width?: string | number
  height?: string | number
  borderRadius?: string | number
  className?: string
}

export function Skeleton({ width, height = 16, borderRadius, className }: SkeletonProps) {
  const classNames = [styles.skeleton, className].filter(Boolean).join(' ')

  return (
    <div
      className={classNames}
      style={{
        width: typeof width === 'number' ? `${width}px` : width,
        height: typeof height === 'number' ? `${height}px` : height,
        borderRadius:
          borderRadius !== undefined
            ? typeof borderRadius === 'number'
              ? `${borderRadius}px`
              : borderRadius
            : undefined,
      }}
    />
  )
}
