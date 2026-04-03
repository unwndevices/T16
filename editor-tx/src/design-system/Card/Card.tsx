import { type ReactNode } from 'react'
import styles from './Card.module.css'

interface CardProps {
  children: ReactNode
  className?: string
  title?: string
  ref?: React.Ref<HTMLDivElement>
}

export function Card({ children, className, title, ref }: CardProps) {
  const classNames = [styles.card, className].filter(Boolean).join(' ')

  return (
    <div ref={ref} className={classNames}>
      {title ? <h3 className={styles.title}>{title}</h3> : null}
      {children}
    </div>
  )
}
