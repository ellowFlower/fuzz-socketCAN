# initializing string
test_str = "hello"

# Converting String to binary
res = ''.join(format(ord(i), '08b') for i in test_str)

# printing result
print("The string after binary conversion : " + str(res))
