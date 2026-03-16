import sys

def main():
    if len(sys.argv) != 3:
        print_help()
        return

    N = int(sys.argv[1])
    size = ((N // 2) + 1) * N
    blocks = int(sys.argv[2])

    points = [
        (l % N, l // N)
        for l in range(size)
    ]
    print("Original:")
    print_grid(points, N)

    block_size = size // blocks
    new_points = []
    for idx in range(block_size):
        for block_idx in range(blocks):
            l = (block_idx * block_size) + idx
            new_points.append((l%N, l//N))

    print("Partitioned:")
    print_grid(new_points, N)


def print_grid(points, row_size):
    c = 0
    for p in points:
        print(f"({p[0]},{p[1]}) ", end='')
        if c == (row_size - 1):
            print("")
        c = (c + 1) % row_size

def print_help():
    print(f"Usuage: {sys.argv[0]} <N> <blocks>")

if __name__ == '__main__':
    main()
