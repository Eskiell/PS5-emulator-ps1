# Instruções para agentes

- Leia este arquivo antes de trabalhar neste repositório.
- O nome do nosso projeto é **PSX1**. O instalador se chama **PSX1 Installer** e seu ELF é `PSX1Installer.elf`. Use esses nomes em novas referências ao projeto e ao instalador; preserve identificadores técnicos existentes quando forem necessários para compatibilidade com o aplicativo e as releases.
- Antes de qualquer ação de trabalho, apresente um plano específico e aguarde aprovação explícita do usuário. Execute somente as ações previstas no plano aprovado; se o escopo mudar, apresente um novo plano e aguarde nova aprovação.
- Cada feature ou melhoria deve ser desenvolvida em uma branch separada, criada a partir de `dev`, e entregue por pull request com base em `dev`.
- Preserve `main` para sincronização com `SynoPiia/PSXS5`; não use `main` como base de pull requests do projeto próprio.
- No FTP `192.168.15.178:2121`, acesse somente logs e somente em modo de leitura. É permitido listar diretórios de logs e baixar ou ler arquivos de log. Nunca escreva, envie, apague, renomeie, mova ou altere arquivos, diretórios, permissões ou metadados nesse FTP.
