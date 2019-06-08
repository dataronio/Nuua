fun rec_fib(n: int): int {
    if n < 2 => return n
    return rec_fib(n - 2) + rec_fib(n - 1)
}

fun iter_fib(n: int): int {
    if n < 2 => return n

    second_fib := 0
    first_fib := 1
    current_fib := 0

    i: int = 2
    while i <= n {
        current_fib = second_fib+first_fib
        second_fib = first_fib
        first_fib = current_fib
        i = i + 1
    }
    return current_fib
}

fun main(argv: [string]) {
    print iter_fib(65)
}
