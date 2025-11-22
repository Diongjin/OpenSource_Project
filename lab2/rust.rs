use std::io;

fn main(){
    print!("행과 열을 입력하세요 : ");

    let mut size = String ::new();
    io::stdin().read_line(&mut size).unwrap();

    let nums: Vec<usize> = size
        .split_whitespace()
        .map(|x| x.parse().unwrap())
        .collect();

    let rows = nums[0];
    let cols = nums[1];
    let total = rows * cols;

    // 첫 번째 행렬 입력
    println!("첫 번째 행렬 값을 입력하세요:");
    let a = read_matrix(total);

    // 두 번째 행렬 입력
    println!("두 번째 행렬 값을 입력하세요:");
    let b = read_matrix(total);

    // 두 행렬 더하기
    let mut result = Vec::with_capacity(total);
    for i in 0..total {
        result.push(a[i] + b[i]);
    }

    // 출력
    println!("\n=== 결과 행렬 ===");
    for r in 0..rows {
        for c in 0..cols {
            print!("{:4}", result[r * cols + c]);
        }
        println!();
    }
}

// 행렬 입력을 받아 Vec<i32>로 저장하는 함수
fn read_matrix(total: usize) -> Vec<i32> {
    let mut v = Vec::with_capacity(total);

    while v.len() < total {
        let mut line = String::new();
        io::stdin().read_line(&mut line).unwrap();

        for num in line.split_whitespace() {
            if let Ok(n) = num.parse::<i32>() {
                v.push(n);
            }
            if v.len() == total {
                break;
            }
        }
    }

    v
}