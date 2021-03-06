Конспект лекции
====

Модель OSI
---

* Транспортный уровень - TCP
* Сеансовый уровень - SSH
* Прикладной уровень - SSL, HTTP, и т.д.
* Не описывает правильно существующие реальности
* Существуют порты

Модель TCP/IP
---

* Сетвеой доступ - hardware
* Прикладной уровень - сокет, данные

Ethernet
---

* Во фрейме может быть до ***1500 байт***
* Присутствет ***checksum 4 bytes***

Path Mtu Discovery
---

* MTU-максимальный кадр передачи данных
* Если он превышен, то данные отбрасываются и отправляется флаг об этом в ip-пакете
* Есть фрагментация кадров для решения этой проблемы, который тормозит передачу даных (надо хэши пересчитывать роутеру)
* Если знаем mtu, то можно убрать фрагментацию

IPV4
---

* icmp контролирует целостность данных
* tracerout - видим маршрут у пакет (время задержки)
* ttl (time to live) - сколько еще осталось "жить" пакету припередачи от сервера к компьютеру
* destination unreachable - не достучать до цели (сети)
* host unreachable - не достучатсья до хоста

NAT
---

* NAT - network address translation
* Преобразование ip-адреса отправителя на тот, который уйдет в сеть
* 192.168.0.0 - не уходит в сеть
* 127.0.0.0 - это хост
* 10.0.0.0 - не уходит в сеть

IPV6
---

* Нет чексуммы
* Нет NAT

ARP
---

* Подмена MAC-адреса на IP внутри сети gateway\ем
* Перенаправление пакетов по MAC-адресу
* Broadcast - это поисковик MAC-адреса. Отправляет запрос всем MAC-ам и они отправляют свои MAC-и.
* У кого такой IP?

DHCP
---

* Протокол, который позволяет назначать IP-адреса внутри сети и широковещательный адрес

UDP
---

* Длина датаграммы может быть больше mtu, но не надо
* Ненадежный протокол
* В контрольной сумме проверяются данные и длина
* Не ясно в каком порядке приходят данные
* Пакет может потеряться
* Приложение высчитывает полностью по одной датаграмме
* Он быстрее TCP

DNS
---

* Его главной хадачей является разрешение символьных имео поверх ip-адреса
* DNS хэшируется, поэтому он разлетиться по сети не сразу
* Клиенты могут достучаться по новому, так и по старому адресу
* команда "dig"
* 8:8:8:8 или 8:8:4:4 сервер DNS
* Можно хранить mx-запись, который отвечает за прием почты на этом домене.
 host emx.mail.ru
* Записи - это данные об ip, куда кидать почту для данного домена и т.д. (A, mx)

TCP
---

* Позволяет адресовать порты
* Нет понятия пакета данных
* Вместо него есть поток данных
* Есть контрольная сумма
* SYN стартует с 0 до 2^32 (случайно)
* SYN(0) -> ACK (1) ->

----

***Заметки по слайдам***
---

* Если адрес 192.168.1.1 и маска 24, то 192.168.1.0 - это адрес сети, 1-254 подсеть и 192.168.1.255 - это широковещательный.
* mac-адрес сохраняется внутри сети. когда пакет выходит из сети, то mac меняется на другой (например на роутера)
* apache работает на форках

----

***Ключевые слова***
---

* icmp
* ttl
* traceroute
* nat
* маска сети
* delayed ACK
* selective ACK