# Top-level constants and variables
MAX_RETRY = 3
$global_var = "global"
TIMEOUT = 30

# Module with nested class and constants
module Networking
  DEFAULT_PORT = 8080

  class Connection
    attr_accessor :host, :port

    @@instance_count = 0

    def initialize(host, port = DEFAULT_PORT)
      @host = host
      @port = port
      @@instance_count += 1
    end

    def self.instance_count
      @@instance_count
    end

    def connect
      # connect logic
    end

    def disconnect
    end

    private

    def authenticate
    end

    protected

    def validate
    end
  end

  module Utils
    def self.parse_url(url)
      url.strip
    end

    def self.encode(data)
      data.to_s
    end
  end
end

# Top-level class
class Animal
  attr_reader :name

  def initialize(name)
    @name = name
  end

  def speak
    raise NotImplementedError
  end
end

class Dog < Animal
  def initialize(name, breed)
    super(name)
    @breed = breed
  end

  def speak
    "Woof!"
  end

  def fetch(item)
    "#{@name} fetches #{item}"
  end
end

# Mixin module
module Greetable
  def greet
    "Hello, I am #{name}"
  end

  def farewell
    "Goodbye from #{name}"
  end
end

# Singleton methods
def format_name(first, last)
  "#{first} #{last}"
end

# Struct
Point = Struct.new(:x, :y)
