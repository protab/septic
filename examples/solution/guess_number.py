low = 1
high = 100

while True:
    my_number = (low + high) // 2
    result = guess(my_number)
    if result == 'too low':
        low = my_number + 1
    elif result == 'too high':
        high = my_number - 1
    else:
        break

print('The secret number is', my_number)
