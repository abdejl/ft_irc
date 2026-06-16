# ft_irc — إصلاح الذاكرة ديال القنوات والتوجيه

## المشكلة

الكلايونتات كانو يقدرو يسجلو ويتكونكتيو بلا مشاكل، ولكن التواصل داخل القنوات كان مخربط بالكامل:

1. **العزل** — الرسائل اللي تتبعثو في قناة ما كانتش توصل للكلايونتات الأخرين فنفس القناة.
2. **الطرد الصامت** — الكلايونت كيتطرد من القناة فالحال ملي كيبعث رسالة أو يدير أي حاجة.

## أصل المشاكل

### 1. الميموري مفصولة (بوغ الواقعتين)

`commandDispatcher::handleJoin` كانت كتخلق أوبجاكت ديال `new Channel()` وكتحفظهم **غير في الفيكتورات الخاصة بيها** (`_vChannelNames`, `_vChannels`). هاد الشي ما كانش يتسجل أبدا في أوبجاكت `Server`.

في نفس الوقت، `handlePrivmsg` كانت كتقلب على القنوات عبر `server.getChannelByName()`، اللي كانت **ستاب كترجع NULL ديما**. الكلاس `Server` ما عندها **أي تخزين للقنوات** — لا ماب، لا فيكتور، والو.

النتيجة: جوج كلايونتات يجوينو `#foo` كل واحد كيتعطاه `Channel*` مختلف. بحال كانو يهضرو فجوج واقعات مختلفات ديال نفس القناة.

### 2. الليفسيكل مقلوب

`handleJoin` ما كانتش تسمي أبدا `chan->addClient(&client)`. حتى لو كان أوبجاكت القناة موجود، ما كان يتضاف فيه أي كلايونت. ملي `handlePrivmsg` كتشيك `chan->hasClient(&client)`، كترجع فولس ديما، وكيجي الإيرور "442 راك ماشي فهاد القناة" — بحال الكلايونت كيتطرد فالحال.

زيادة على هاد الشي، السطر 189 كيستعمل `return` عوض `continue`، اللي كيخرج من الفونكسيون بالكامل ملي كيلقا قناة مكررة عوض ما يسكتها ويمشي للقناة الجاية.

### 3. الفورمات ديال البرودكاست مهاردكود

`Channel::broadcast()` كانت ديما كتزيد `:nick PRIVMSG #channel :` قبل كل رسالة، حتى رسائل JOIN. هاد الشي كيكسر البروتوكول ديال IRC للرسائل اللي ماشيش PRIVMSG.

## الحل

### `parsing/Client.hpp` — السيرفر دير ملكية القنوات

```cpp
class Server{
private:
    std::map<std::string, Channel*> _channels;  // جديد: مصدر واحد للحقيقة
public:
    Channel* getOrCreateChannel(const std::string& name);  // جديد
    // ... باقي الميتودات ماشي متبدلة
};
```

### `parsing/Client.cpp` — الميتودات ديال السيرفر تتيمبليمنتيو

**`getOrCreateChannel`** — تقلب على قناة موجودة أو تخلق واحدة جديدة في الماب ديال السيرفر:
```cpp
Channel* Server::getOrCreateChannel(const std::string& name)
{
    std::map<std::string, Channel*>::iterator it = _channels.find(name);
    if (it != _channels.end())
        return it->second;
    Channel *chan = new Channel();
    chan->setName(name);
    _channels[name] = chan;
    return chan;
}
```

**`getChannelByName`** — دابا كتقلب في الماب فعلا عوض ما ترجع NULL.

**`processChannelJoin`** — كتهندل كامل عملية الجوين:
1. تقلب أو تخلق القناة في `Server::_channels`
2. تدير شيكات `canJoin()` (الكي، الليميت، الإنفايت)
3. تسمي `addClient()` باش تزيد الكلايونت للقناة
4. تديه أوبيراتور إلا كانت القناة خاوية (أول واحد يدخل)
5. تبعث تأكيد JOIN للكلايونت اللي دخل
6. تبرودكاست JOIN للأعضاء اللي كانو فالقناة

### `parsing/cmdDispatcher.hpp` — الديسباتشر تنقى من الستيت

تحذف المامبيرات الخاصة:
```diff
 class commandDispatcher{
-private:
-    std::vector<std::string> _vChannelNames;
-    std::vector<Channel *> _vChannels;
-public:
+public:
```

الديسباتشر دابا ما عندوش أي داتا ديال القنوات. هو غير فونكسيونال بحت.

### `parsing/cmdDispatcher.cpp` — handleJoin تبسطات

قبل: 60 سطر ديال تخصيص القنوات وديتيكسيون المكرر والبرودكاست المكسور.

بعد: غير بارسينج خالص وتفويض لـ `server.processChannelJoin()`:
```cpp
void commandDispatcher::handleJoin(Client &client, const Command &cmd, Server &server)
{
    // ... تشيك التسجيل، بارسينج الباراميترات، سبليت بالفاصلة ...
    for (size_t i = 0; i < channels.size(); i++)
    {
        // ... تفاليد البريفيكس ديال القناة (# أو &) ...
        server.processChannelJoin(client, channelName, key);
    }
}
```

**`handlePrivmsg`** — دابا كتبني كامل سطر PRIVMSG ديال IRC وكتبعثو عبر `Channel::broadcast()`. بما أن `getChannelByName` كتخدم مزيان، الرسائل كتوصل.

### `channel/channel.cpp` — البرودكاست تصلحات

**`broadcast`** دابا كتبعث غير الستريينغ كما هو — كتبعث اللي يجيها من الكولير:
```cpp
void Channel::broadcast(std::string message, Client *sender)
{
    for (std::vector<Client*>::iterator it = _vClient.begin(); it != _vClient.end(); it++)
    {
        if ((*it)->getFd() != sender->getFd())
            send((*it)->getFd(), message.c_str(), message.length(), 0);
    }
}
```

كل الكوليرات (`kickClient`، `ChangeTopic`، `removeClient`، `inviteToChannel`) دابا كيفورماتيو سطور البروتوكول ديال IRC الكاملة قبل ما يسميو `broadcast`.

## الأركيتيكتشر بعد الإصلاح

```
طبقة الديسباتشر (بلا ستيت)              محرك السيرفر (مصدر واحد للحقيقة)
┌─────────────────────────┐          ┌──────────────────────────────┐
│ handleJoin:             │          │ _channels: map<string, Chan*>│
│   بارسينج أسماء القنوات │────────> │   "#foo" -> Channel* (مشترك) │
│   بارسينج الكيوات       │  تفويض  │   "#bar" -> Channel* (مشترك) │
│   تفاليد البريفيكس      │          │                              │
│                         │          │ processChannelJoin():        │
│ handlePrivmsg:          │          │   getOrCreateChannel()       │
│   بارسينج التارقيت      │────────> │   شيك canJoin()              │
│   بارسينج الرسالة       │  بحث    │   addClient()                │
│   فورماتينج سطر IRC     │          │   برودكاست JOIN              │
│   إرسال عبر القناة      │<──────── │                              │
└─────────────────────────┘          └──────────────────────────────┘
```

## البيلد

```bash
make re    # ريبيلد نظيف
./ircserv <port> <password>
```

كامل الكود كيكومبيل تحت `-Wall -Wextra -Werror -std=c++98` بزيرو وارنينغات.
