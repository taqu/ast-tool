package com.example.app

import scala.collection.mutable

object Constants {
  val MaxSize: Int = 100
  val Pi: Double = 3.14159
  var counter: Int = 0
}

class Animal(val name: String, protected val age: Int) {
  val sound: String = "..."

  def speak(): Unit = {
    println(s"$name says $sound")
  }

  def getName(): String = name

  protected def internalCheck(): Boolean = true

  private def secret(): Unit = ()
}

class Dog(name: String, age: Int) extends Animal(name, age) {
  override val sound: String = "Woof"

  override def speak(): Unit = {
    println(s"$name barks!")
  }

  def fetch(item: String): String = s"$name fetched $item"
}

abstract class Shape {
  def area(): Double
  def perimeter(): Double
  val color: String = "red"
}

trait Printable {
  def print(): Unit
  def prettyPrint(): Unit = println(toString)
}

trait Serializable {
  def serialize(): String
  def deserialize(data: String): Unit
}

object MathUtils {
  def add(a: Int, b: Int): Int = a + b
  def subtract(a: Int, b: Int): Int = a - b

  object Trig {
    val Pi: Double = math.Pi
    def sin(x: Double): Double = math.sin(x)
    def cos(x: Double): Double = math.cos(x)
  }
}

case class Point(x: Double, y: Double) {
  def distanceTo(other: Point): Double = {
    math.sqrt(math.pow(x - other.x, 2) + math.pow(y - other.y, 2))
  }
}

case class Rectangle(width: Double, height: Double) extends Shape with Printable {
  override def area(): Double = width * height
  override def perimeter(): Double = 2 * (width + height)
  override def print(): Unit = println(s"Rectangle($width x $height)")
}

sealed trait Color
case object Red extends Color
case object Green extends Color
case object Blue extends Color
case class Custom(r: Int, g: Int, b: Int) extends Color

sealed abstract class Result[+A]
case class Success[+A](value: A) extends Result[A]
case class Failure(error: String) extends Result[Nothing]

class Container[T](private var value: T) {
  def get: T = value
  def set(v: T): Unit = { value = v }
}

object Main {
  type Callback = () => Unit
  type StringMap = Map[String, String]

  def main(args: Array[String]): Unit = {
    val dog = new Dog("Rex", 3)
    dog.speak()
  }
}

package object utils {
  def greet(name: String): String = s"Hello, $name!"
  val version: String = "1.0.0"
}
