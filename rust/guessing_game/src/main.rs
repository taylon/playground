use std::cmp::Ordering;
use std::io::{stdin, stdout, BufWriter};
use std::num::ParseIntError;
use std::result::Result;

use ferris_says::say;
use rand::Rng;

fn ask_for_number() -> Result<u32, ParseIntError> {
    let mut guess = String::new();
    stdin().read_line(&mut guess).expect("Failed to read line");

    guess.trim().parse()
}

fn print_victory_message(message: String) {
    let stdout = stdout();
    let width = message.chars().count();
    let mut writer = BufWriter::new(stdout.lock());

    say(message.as_bytes(), width, &mut writer).unwrap();
}

fn generate_random_number() -> u32 {
    rand::thread_rng().gen_range(1, 101)
}

fn main() {
    println!("Guess the number!");

    let random_number = generate_random_number();
    println!("The secret number is: {}\n", random_number);

    loop {
        println!("Please input your guess.");
        let guess = match ask_for_number() {
            Ok(num) => num,
            Err(_) => {
                println!("Invalid input! Only numbers are allowed.");
                continue;
            }
        };

        println!("\nYou guessed: {}", guess);

        match guess.cmp(&random_number) {
            Ordering::Less => println!("Too small!"),
            Ordering::Greater => println!("Too big!"),
            Ordering::Equal => {
                print_victory_message("You Win!".to_string());
                break;
            }
        }
    }
}
