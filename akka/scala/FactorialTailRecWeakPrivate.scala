//-*- coding: utf-8-unix -*-
/**
 * (C) IT Sky Consulting GmbH 2014
 * http://www.it-sky-consulting.com/
 * Author: Karl Brodowsky
 * Date: 2014-05-20
 * License: GPL v2 (See https://de.wikipedia.org/wiki/GNU_General_Public_License )
 *
 */
import scala.annotation.tailrec

object FactorialTailRecWeakPrivate {
  def main(args : Array[String]) : Unit = {
    val n : BigInt = BigInt(args(0))
    val f : FactorialTailRecWeakPrivate = new FactorialTailRecWeakPrivate()
    val fOfN : BigInt = f.factorial(n)
    println("" + n + "!=" + fOfN);
  }
}

class FactorialTailRecWeakPrivate {
  def factorial(n : BigInt) : BigInt = {
    factorial2(n, 1)
  }
  
  @tailrec
  private def factorial2(n : BigInt, partial : BigInt) : BigInt = {
    if (n <= 1) {
      partial
    } else {
      factorial2(n - 1, partial * n)
    }
  }
}
