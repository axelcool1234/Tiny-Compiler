def mod(a, b):
    while a >= b:
        a -= b
    return a

def is_prime(num):
    if num < 2:
        return False
    i = 2
    while i * i <= num:
        if mod(num, i) == 0:
            return False
        i += 1
    return True

def count_distinct_prime_factors(num):
    count = 0
    factor = 2
    while num > 1:
        if is_prime(factor):
            if mod(num, factor) == 0:
                count += 1
                while mod(num, factor) == 0:
                    num //= factor
        factor += 1
    return count

def main():
    n = 100
    m = 3
    num = 2
    while num <= n:
        if count_distinct_prime_factors(num) == m:
            print(num)
        num += 1

if __name__ == "__main__":
    main()
