# Carrinho auxiliador de supermercado

Para rodar:

- Clone o projeto através do botão clonar
- Abra o projeto no [Visual Studio Code](https://code.visualstudio.com/download)
- Instale a extensão [Platform.io](https://docs.platformio.org/en/latest/integration/ide/vscode.html)
- [Configure o upload para SPIFFS](https://randomnerdtutorials.com/esp32-vs-code-platformio-spiffs)



Com isso, basta abrir o VSCode e clicar no menu lateral no Platform.io, selecionar a opção de abrir um projeto e selecionar o "carrinho-auxiliador-supermercado".


Para subir uma nova versão da página web, você precisa fazer o build e o upload de um "Filesystem Image", que é ele quem sobe os arquivos para a memória da ESP32. Para aprender a fazer isso, acesse o link de configuaração dos SPIFFS.

Para subir uma versão de código apenas (do codigo .ino do controlador) não é necessário gerar um FileSystem Image,  apenas fazer o build e upload do código!
