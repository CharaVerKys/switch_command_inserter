осталось:  
    2.сделать identify на regex (добавить конфиг файл и сделать похожую обработку как у commit)
(разница в том что несколько попыток авторизации пока пароли не закончатся, или пока не дропнит подключение удалённый хост)
  (и то что обход набора команд реализован как двумерный вектор, где результат совпадения - запись модель в host.model)

в остальном программа полностью работает, дальше делать telnet сессии для identify и commit


особенности которые обязательно в документации написать:
	1. нельзя отправлять ? в некоторых коммутаторах ( в частности cisco ) = приводит к тому что программа не сможет корректно считать следующий вывод (точнее можно, но если это последняя команда которую вы хотите отправить)
	2. в прошивке cisco для show run и прочих с --More-- требуется ввести term le 0, иначе будет получен не полный вывод команды
  3. таймаут подключения - 15 минут в течении которых программа не может считать ответ от удалённого хоста (для каждого хоста отдельный)
  4. 