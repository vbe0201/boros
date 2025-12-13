from boros import _impl
import types

conf = _impl.RunConfig()
conf.sq_size = 2

#async def main():
#    for i in range(3):
#        res = await _impl.nop(i)
#        print(res)


@types.coroutine
def a():
    yield

async def main():
    try:
        await _impl.nop(1)
    finally:
        print("we are in Python land")


x = _impl.run(main(), conf)
print(x)
