// Module-level const/let/var
const PI = 3.14;
let counter = 0;
var legacy = true;

// Arrow function assigned to const
const add = (a, b) => a + b;
const greet = function(name) { return `Hello ${name}`; };

// Regular function
function helper(x) { return x * 2; }

// Class with fields, methods, static members
class Animal {
    #name;
    static count = 0;

    constructor(name) {
        this.#name = name;
        Animal.count++;
    }

    get name() { return this.#name; }
    set name(v) { this.#name = v; }

    speak() { return `${this.#name} speaks`; }

    static create(name) { return new Animal(name); }
}

class Dog extends Animal {
    #breed;

    constructor(name, breed) {
        super(name);
        this.#breed = breed;
    }

    speak() { return `Woof from ${this.name}`; }
}

// Object literal at module scope
const utils = {
    format(x) { return String(x); },
    parse: function(s) { return parseInt(s); },
};

// Generator function
function* idGen() { let id = 0; while(true) yield id++; }

// Async function
async function fetchData(url) { return await fetch(url); }

// Named export
export function exported() { return 42; }
export class ExportedClass { method() {} }
export const CONSTANT = 'value';

// Default export
export default class DefaultClass {
    run() {}
}
