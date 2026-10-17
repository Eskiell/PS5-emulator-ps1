# Playstation 1

Emulador de PlayStation 1 para PS5 com jailbreak.

## Instalação

1. Prepare o PS5 para executar homebrew, com acesso a `/data` e ShadowMountPlus.
2. Baixe `PSXS5Installer.elf` em [Releases](https://github.com/Eskiell/PS5-emulator-ps1/releases) e execute-o pelo carregador de payloads. Ele instala ou atualiza o aplicativo em `/data/homebrew/PPSA97510/`.
3. Coloque seus jogos em `/data/PSXS5/games/<jogo>/` e, opcionalmente, uma BIOS própria em `/data/PSXS5/bios/`. Reinicie ShadowMountPlus ou o console para exibir o aplicativo.

O instalador preserva jogos, BIOS, saves e configurações. Veja os [detalhes do instalador](installer/README.md).

## Código-fonte

`make` compila o aplicativo; `make installer` compila o ELF do instalador.

## Créditos e licença

Baseado no [PSXS5 de SynoPiia](https://github.com/SynoPiia/PSXS5). Os créditos e as licenças das dependências estão em [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). O projeto usa a licença [GPL-3.0-or-later](LICENSE). BIOS e jogos não são distribuídos.
