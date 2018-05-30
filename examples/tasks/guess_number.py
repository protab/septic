import random

secret = random.randint(1, 100)

@export
def guess(number):
    number = int(number)
    if number < secret:
        return 'too low'
    if number > secret:
        return 'too high'
    return 'you won!'
