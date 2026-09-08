class Animal {
    #name;
    species = "unknown";

    constructor(name) {
        this.#name = name;
    }

    speak() {
        return "...";
    }

    static create() {}
}
