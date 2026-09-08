class Empty:
    pass


def outer():
    def inner():
        pass
    local = 42
    return local


class WithStaticMethod:
    @staticmethod
    def static_method():
        pass
