// TypeScript comprehensive test file

// Enums
enum Direction {
    Up,
    Down,
    Left,
    Right
}

const enum Status {
    Active = "ACTIVE",
    Inactive = "INACTIVE"
}

// Interfaces
interface Animal {
    name: string;
    age: number;
    speak(): void;
    move?(distance: number): void;
}

interface Repository<T> {
    findById(id: number): T;
    save(entity: T): void;
}

// Type aliases
type Callback = () => void;
type StringMap = Map<string, string>;
type Handler<T> = (event: T) => void;

// Namespaces
namespace Geometry {
    export interface Point {
        x: number;
        y: number;
    }

    export class Circle {
        constructor(public center: Point, public radius: number) {}

        area(): number {
            return Math.PI * this.radius ** 2;
        }
    }

    export function distance(a: Point, b: Point): number {
        return Math.sqrt((a.x - b.x) ** 2 + (a.y - b.y) ** 2);
    }

    export const PI = Math.PI;
}

// Classes
class Person implements Animal {
    private static count: number = 0;
    protected id: number;
    public name: string;
    readonly birthYear: number;
    #secret: string = "hidden";

    constructor(name: string, birthYear: number) {
        this.name = name;
        this.birthYear = birthYear;
        this.id = ++Person.count;
        this.#secret = name;
    }

    speak(): void {
        console.log(`${this.name} speaks`);
    }

    move(distance: number = 0): void {
        console.log(`${this.name} moved ${distance}m`);
    }

    get age(): number {
        return new Date().getFullYear() - this.birthYear;
    }

    set displayName(value: string) {
        this.name = value;
    }

    static getCount(): number {
        return Person.count;
    }
}

abstract class Shape {
    abstract area(): number;
    abstract perimeter(): number;

    describe(): string {
        return `Area: ${this.area()}`;
    }
}

class Rectangle extends Shape {
    constructor(public width: number, public height: number) {
        super();
    }

    area(): number {
        return this.width * this.height;
    }

    perimeter(): number {
        return 2 * (this.width + this.height);
    }
}

// Generic class
class Stack<T> {
    private items: T[] = [];

    push(item: T): void {
        this.items.push(item);
    }

    pop(): T | undefined {
        return this.items.pop();
    }

    get size(): number {
        return this.items.length;
    }
}

// Decorators (class and method)
function sealed(constructor: Function): void {}
function log(target: any, key: string, descriptor: PropertyDescriptor) {}

@sealed
class BankAccount {
    private balance: number = 0;

    @log
    deposit(amount: number): void {
        this.balance += amount;
    }

    withdraw(amount: number): boolean {
        if (this.balance >= amount) {
            this.balance -= amount;
            return true;
        }
        return false;
    }
}

// Free functions
function greet(name: string): string {
    return `Hello, ${name}!`;
}

async function fetchData(url: string): Promise<string> {
    return "";
}

function* idGenerator(): Generator<number> {
    let id = 0;
    while (true) yield ++id;
}

// Variables
const MAX_SIZE: number = 100;
let currentUser: string = "admin";
var legacyFlag: boolean = false;

// Arrow functions as variables
const add = (a: number, b: number): number => a + b;
const multiply = (a: number, b: number) => a * b;

// Object with typed methods
const mathUtils = {
    square(x: number): number { return x * x; },
    cube: (x: number): number => x * x * x
};

// Module augmentation / ambient declarations
declare namespace NodeJS {
    interface ProcessEnv {
        API_KEY: string;
        PORT: string;
    }
}

// Export patterns (transparent — inner declarations are the symbols)
export { greet, MAX_SIZE };
export default class DefaultExport {
    run(): void {}
}
