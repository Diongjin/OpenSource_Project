use std::collections::HashMap;
use std::io;

fn main(){
    let mut phone: HashMap<String,String>  = HashMap::new();

    loop{
        println!("\n -- 메뉴 --");
        println!("1. 추가");
        println!("2. 검색");
        println!("3. 종료");

        let pick = read_line().trim().to_string();

        match pick.as_str(){
            "1" => add(&mut phone),
            "2" => find(&phone),
            "3" => {
                println!("프로그램 종료!");
                break;
            }
            _ => println!("잘못된 입력입니다."),
        }
    }
}


fn add(p : &mut HashMap<String,String>){
    println!("이름 입력 : ");
    let name = read_line().trim().to_string();

    println!("전화번호 입력 : ");
    let number = read_line().trim().to_string();

    p.insert(name.clone(),number.clone());
    println!("저장 완료: {} -> {}",name,number);
}

fn find(p:&HashMap<String, String>) {
    println!("이름 검색:");
    let name = read_line().trim().to_string();

    match p.get(&name) {
        Some(number) => println!("{}의 번호: {}", name, number),
        None => println!("해당 이름이 존재하지 않습니다."),
    }
}

fn read_line() -> String {
    let mut s = String::new();
    io::stdin().read_line(&mut s).unwrap();
    s
}