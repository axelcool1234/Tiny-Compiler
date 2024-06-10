def output_num(num):
    print(num, end='')

def output_newline():
    print()

def mandelbrot(x, y):
    x0 = x
    y0 = y
    iters = 0
    go = 1
    while go != 0:
        if x * x + y * y > 4 * 10000 * 10000:
            go = 0
        if iters >= 100:
            go = 0
        if go != 0:
            x2 = (x * x - y * y) // 10000 + x0
            y = (2 * x * y) // 10000 + y0
            x = x2
            iters += 1
    return iters

def main():
    px = 0
    py = 0
    while py < 200:
        px = 0
        while px < 200:
            mval = mandelbrot(((px - 100) * 4 * 10000) // 200, ((py - 100) * 4 * 10000) // 200)
            if mval == 100:
                output_num(8)
            else:
                output_num(1)
            px += 1
        py += 1
        output_newline()

if __name__ == "__main__":
    main()
