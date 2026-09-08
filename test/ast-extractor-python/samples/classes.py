class Animal:
    species = "Unknown"

    def __init__(self, name):
        self.name = name

    def speak(self):
        pass

    def move(self):
        pass


class Dog(Animal):
    def speak(self):
        return "Woof!"

    def fetch(self):
        pass
