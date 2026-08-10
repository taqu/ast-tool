# frozen_string_literal: true

# Module-level documentation for Coding Agent testing.

# Manages user session and authentication.
class UserManager
  attr_reader :db_url

  def initialize(db_url)
    @db_url = db_url
  end

  def get_user_by_id(user_id)
    # TODO: Implement database lookup
    nil
  end
end

def main
  # Ruby 3.0+ パターンマッチング (case in) のテスト
  status = 200
  case status
  in 200 | 201
    puts "Success"
  else
    puts "Failure"
  end
end

if __FILE__ == $PROGRAM_NAME
  main
end
