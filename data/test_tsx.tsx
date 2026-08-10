import React, { useState, useEffect, useContext } from 'react';

// Types and interfaces
interface ButtonProps {
    label: string;
    onClick: () => void;
    disabled?: boolean;
    variant?: 'primary' | 'secondary';
}

interface CardProps {
    title: string;
    children: React.ReactNode;
    className?: string;
}

type Theme = 'light' | 'dark';
type EventHandler<T> = (event: T) => void;

// Context
const ThemeContext = React.createContext<Theme>('light');

// Enums
enum ButtonVariant {
    Primary = 'primary',
    Secondary = 'secondary',
    Danger = 'danger'
}

// Simple functional component (arrow function)
const Button = ({ label, onClick, disabled = false }: ButtonProps): JSX.Element => {
    return <button onClick={onClick} disabled={disabled}>{label}</button>;
};

// Functional component with hooks
const Card = ({ title, children, className }: CardProps): JSX.Element => {
    const [isVisible, setIsVisible] = useState(true);

    useEffect(() => {
        document.title = title;
    }, [title]);

    return (
        <div className={className}>
            <h2>{title}</h2>
            {isVisible && children}
        </div>
    );
};

// Function declaration component
function Header({ title }: { title: string }): JSX.Element {
    return <header><h1>{title}</h1></header>;
}

// Class component
class Counter extends React.Component<{ initialCount: number }, { count: number }> {
    private intervalId: number = 0;

    constructor(props: { initialCount: number }) {
        super(props);
        this.state = { count: props.initialCount };
    }

    componentDidMount(): void {
        this.intervalId = window.setInterval(() => {
            this.setState(s => ({ count: s.count + 1 }));
        }, 1000);
    }

    componentWillUnmount(): void {
        clearInterval(this.intervalId);
    }

    increment = (): void => {
        this.setState(s => ({ count: s.count + 1 }));
    };

    render(): JSX.Element {
        return (
            <div>
                <p>Count: {this.state.count}</p>
                <button onClick={this.increment}>+</button>
            </div>
        );
    }
}

// Generic component
function List<T extends { id: number; name: string }>(
    { items }: { items: T[] }
): JSX.Element {
    return (
        <ul>
            {items.map(item => <li key={item.id}>{item.name}</li>)}
        </ul>
    );
}

// Custom hook
function useCounter(initial: number = 0) {
    const [count, setCount] = useState(initial);
    const increment = () => setCount(c => c + 1);
    const decrement = () => setCount(c => c - 1);
    const reset = () => setCount(initial);
    return { count, increment, decrement, reset };
}

// Higher-order component
function withTheme<P extends object>(
    WrappedComponent: React.ComponentType<P>
) {
    return function ThemedComponent(props: P): JSX.Element {
        const theme = useContext(ThemeContext);
        return <WrappedComponent {...props} theme={theme} />;
    };
}

// Namespace
namespace Components {
    export interface ModalProps {
        isOpen: boolean;
        onClose: () => void;
    }

    export const Modal = ({ isOpen, onClose }: ModalProps): JSX.Element | null => {
        if (!isOpen) return null;
        return (
            <div className="modal">
                <button onClick={onClose}>Close</button>
            </div>
        );
    };

    export function Tooltip({ text }: { text: string }): JSX.Element {
        return <span title={text} />;
    }
}

// Type alias
type ComponentMap = Record<string, React.ComponentType<any>>;

// Constants
const MAX_RETRIES = 3;
const DEFAULT_THEME: Theme = 'light';

// Utility functions
export function formatLabel(text: string): string {
    return text.trim().toUpperCase();
}

export const createId = (prefix: string): string => `${prefix}-${Date.now()}`;

// Custom element / web component tag detection via JSX
const webComponentExample = (
    <my-button variant="primary">Click me</my-button>
);
