require "socket"
require "thread"
require "ipaddr"
require "securerandom"

require_relative 'message'

class Client
  BIND_ADDR = '<broadcast>'
  PORT = 33333

  def initialize
    @client_id = SecureRandom.hex(5)
  end

  def transmit(content)
    message = Message.new('client_id' => @client_id, 'content' => content)
    socket.send(message.to_json, 0, BIND_ADDR, PORT)
    message
  end


  def listen
    socket.bind(BIND_ADDR, PORT)

    Thread.new do
      loop do
        attributes, _ = socket.recvfrom(1024, 0)
        message = Message.inflate(attributes)
        puts message.sexy unless message.client_id == @client_id
      end
    end

    @listening = true
  end

  def listening?
    @listening == true
  end

  private

  def socket
    @socket ||= UDPSocket.open.tap do |socket|
      socket.setsockopt(:SOL_SOCKET, :SO_BROADCAST, 1)
      socket.setsockopt(:SOL_SOCKET, :SO_REUSEADDR, 1)
    end
  end
end
