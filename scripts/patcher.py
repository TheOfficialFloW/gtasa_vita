def patch(file, offset, data):
    file.seek(offset)
    file.write(bytes.fromhex(data))

def peek(file, offset, bytes):
    file.seek(offset)
    line = file.read(bytes).hex(' ', 1)
    print(line.upper())

def main():
    try:
        with open('mainV1.scm', 'r+b') as scm, open('scriptv1.img', 'r+b') as img:
            # 00E1 -> 0A90 0000
            patch(scm, 0x00066703, '90 0A 04 40 00 00') # ITB
            patch(scm, 0x0006675A, '90 0A 04 40 00 00')
            patch(img, 0x0004C0FF, '90 0A 04 40 00 00') # Gym treadmill
            patch(img, 0x0004C131, '90 8A 04 40 00 00')
            print("Patched successfully!")
    except IOError as ex:
        print(ex)

if __name__ == '__main__':
  exit(main())