class Collection {
    numbers: [int]
    fun map(f: (int -> int)): Collection {
        for element, index in self.numbers {
            self.numbers[index] = f(element)
        }
        return self
    }
}

fun multiply(x: int): int -> x * 2

fun main(argv: [string]) {
    c := Collection!{numbers: [1, 2, 3, 4, 5]}
    c.map(multiply).map(multiply)
    print c.numbers
}
