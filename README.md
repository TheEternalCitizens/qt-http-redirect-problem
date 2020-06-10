# Qt HTTP `POST` to `GET` Redirect Problem

**TL;DR** Mårten Nordheim from the Qt development team fixed the bug [here](https://bugreports.qt.io/browse/QTBUG-84162).

I'm having a problem getting Qt to follow a `POST` request's redirect to a `GET` request. I have a `POST` endpoint that returns a 302 redirect to a `GET` endpoint, and I find that Qt is unable to do this. What Qt does is make the `POST` request, start the `GET` request, but then fail silently.

This git repository contains the code necessary to reproduce this problem: an http server and a Qt http client. The http server is dockerized, so it's only dependencies are docker and docker-compose. I wrote the Qt http client with Qt version 5.14.2, but I think it's backwards compatible all the way back to Qt version 5.9.

## What should happen

First, start the http server via `docker-compose up`. In another shell, use `curl` to see what __should__ happen:
```sh
curl -L 0.0.0.0:4567/login -d "hello=world"
```
#### `curl` output
```sh
<html>
  <body>
    <h1>Overview</h1>

  </body>
</html>
```
#### http server output
```sh
192.168.64.1 - - [03/Jun/2020:19:10:20 +0000] "POST /login HTTP/1.1" 302 - 0.0013
192.168.64.1 - - [03/Jun/2020:19:10:20 UTC] "POST /login HTTP/1.1" 302 0
- -> /login
192.168.64.1 - - [03/Jun/2020:19:10:20 +0000] "GET /overview HTTP/1.1" 200 57 0.0031
192.168.64.1 - - [03/Jun/2020:19:10:20 UTC] "GET /overview HTTP/1.1" 200 57
- -> /overview
```
What happened was that `curl` made a call to `POST /login`, got a 302, and followed it to `GET /overview`, which returned some HTML. To see this in more detail, turn up the verbosity:
```sh
 curl -L -v 0.0.0.0:4567/login -d "hello=world" -D -
```

## What goes wrong

To see what goes wrong when trying this with Qt, run the Qt application with Qt Creator. The Qt UI widget contains a button and a select menu. Pressing the button once launches the request. Pressing it again resets it to prepare another request. The select menu contains all the ["redirect policies" that Qt offers](https://doc.qt.io/qt-5/qnetworkrequest.html#RedirectPolicy-enum). It defaults to "Manual Redirect Policy", meaning that it doesn't follow the redirect. The second and third options, "No Less Safe Redirect Policy" and "Same Origin Redirect Policy", should allow us to follow the redirect when selected. The fourth option requires I prompt the user if we should follow the redirect, which I haven't explored yet.

![image](https://user-images.githubusercontent.com/3466499/83682231-a557a380-a5b1-11ea-859d-c99c7a3956f6.png)

First, try making a request with "Manual Redirect Policy" selected in order to not follow the redirect, and note how it succeeds.
#### http server output
```sh
192.168.64.1 - - [03/Jun/2020:19:17:25 +0000] "POST /login HTTP/1.1" 302 - 0.0006
192.168.64.1 - - [03/Jun/2020:19:17:25 UTC] "POST /login HTTP/1.1" 302 0
- -> /login
```

Now try making a request with "No Less Safe Redirect Policy" selected in order to follow the redirect, and note that it fails.
#### http server output
```sh
192.168.64.1 - - [03/Jun/2020:19:18:56 +0000] "POST /login HTTP/1.1" 302 - 0.0096
192.168.64.1 - - [03/Jun/2020:19:18:56 UTC] "POST /login HTTP/1.1" 302 0
- -> /login
[2020-06-03 19:18:56] ERROR invalid body size.
192.168.64.1 - - [03/Jun/2020:19:18:56 UTC] "GET /overview HTTP/1.1" 400 278
- -> /overview
```

The http server complains that the call to `GET /overview` has an invalid body size, and returns a 400. According to [the source code underlying the http server](https://github.com/ruby/webrick/blob/6b6990ec81479160d53d81310c05ab4dc508b199/lib/webrick/httprequest.rb#L517-L519), that means its socket encountered an EOF. If we use the Qt Creator debugger to step into [underlying Qt networking code](https://github.com/qt/qtbase/blob/3673ee98236f7b901db3112f0112ad57691a2358/src/network/access/qhttpprotocolhandler.cpp#L372-L375), we see that this happens because Qt notices something is wrong and aborts.

So there's __two things__ that are going wrong here. First, the redirect fails. Second, it fails silently. Normally in the event of an error, the QtNetworkAccessManager or QtNetworkReply receive a call to to one of their slot methods. I've logged all the calls they receive to QDebug.

When called when __not__ following redirects, it looks like this:
#### QDebug output
```sh
networkReply uploadProgress
networkReply metaDataChanged
networkReply downloadProgress
networkReply uploadProgress
networkAccessManager finished
networkReply finished
```
When called when following redirects, it looks like this:
#### QDebug output
```sh
networkReply uploadProgress
networkReply metaDataChanged
networkReply redirected
```
The QNetworkReply sees that it gets redirected, but nothing more. Critically, the "finished" slot method on the QNetworkAccessManager never gets called.

## This is a Qt bug!

This is a Qt bug! Or more rightly, this is two Qt bugs. First, the redirect should work, and second, if it doesn't, it should emit a signal explaining its failure.

I see that [someone else has recently reported this problem](https://bugreports.qt.io/browse/QTBUG-84162), so I've added my analysis to their report.

Above, I said that Qt notices something is wrong and aborts. The thing that is wrong is that the data from the `POST` request is still hanging around during the `GET` request. Qt notices this, and goes to write it. However, the data is stored in a buffer that has been played to its end and not rewound, and thus it is not ready to be reused. Qt notices this contradiction [here](https://github.com/qt/qtbase/blob/3673ee98236f7b901db3112f0112ad57691a2358/src/network/access/qhttpprotocolhandler.cpp#L372-L375) and aborts.

But more fundamentally, this data shouldn't be reused at all! When redirecting to a `GET` after a `POST`, we shouldn't try to submit the data for the `POST` endpoint to the `GET` endpoint.

The simplest way I see to fix this bug is to modify this switch statement [here](https://github.com/qt/qtbase/blob/bd3b978701c32b2e13da853f2064aab369e32745/src/network/access/qnetworkreplyhttpimpl.cpp#L684-L724). Every case that doesn't have a `createUploadByteDevice()` (e.g. `GET` requests) should have a `uploadByteDevice = nullptr`, because it's the presence of this upload byte device, carrying over from the `POST` request, that [later causes Qt to inappropriately put the `GET` request into a writing state](https://github.com/qt/qtbase/blob/5a779a4ad350accadc4337d332eedb29ba1cc26b/src/network/access/qhttpprotocolhandler.cpp#L325-L332).

In either case, when Qt is aborting this request, it should emit a signal to let consuming code know what happened. It attempts to do this with `QHttpNetworkConnectionPrivate::emitReplyError(...)` [here](https://github.com/qt/qtbase/blob/5a779a4ad350accadc4337d332eedb29ba1cc26b/src/network/access/qhttpprotocolhandler.cpp#L374), which emits the `QHttpNetworkReply::finishedWithError` signal, which oddly never connects to `QHttpThreadDelegate::finishedWithErrorSlot` [here](https://github.com/qt/qtbase/blob/f66a8edc200482308c8567395a7b6f95143c8f92/src/network/access/qhttpthreaddelegate.cpp#L555-L575), which would have emitted the missing signals. I am not sure how to diagnose or fix this second bug.
