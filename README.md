# Nome do arquivo: meu_terminal.py

def meu_terminal():
    print("Bem-vindo ao Meu Terminal")
    
    # Solicitar senha
    senha = input("Por favor, insira a senha: ")
    
    if senha == "7263":
        print("Senha correta!")
        print("\nQual linguagem minha você prefere?")
        print("1) LONGS (a minha linguagem)")
        print("2) NSODB (a minha outra linguagem)")
        
        escolha = input("Escolha uma opção (1 ou 2): ")
        
        if escolha == "1":
            print("Você escolheu LONGS. Digite 'help' para acessar o aplicativo.")
        elif escolha == "2":
            print("Você escolheu NSODB. Digite 'help' para acessar o aplicativo.")
        else:
            print("Opção inválida. Saindo...")
            return
        
        # Esperar o comando "help"
        comando = input("\nDigite um comando: ")
        if comando.lower() == "help":
            print("Seu aplicativo está pronto! Divirta-se!")
        else:
            print("Comando desconhecido. Saindo...")
    else:
        print("Senha incorreta. Acesso negado.")

# Executa o terminal
if __name__ == "__main__":
nano meu_terminal.py
python3 meu_terminal.py

    meu_terminal()
