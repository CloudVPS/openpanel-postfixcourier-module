import smtplib, poplib, subprocess, random, time, string

requires=['Domain']

def postmapq(mapname, key):
    return subprocess.Popen([
        'postmap',
        '-q',
        key,
        '/etc/postfix/openpanel/'+mapname
        ], stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()

def smtpauthcheck(ctx, box, password):
    s = smtplib.SMTP()
    s.connect()
    try:
        ctx.logger.debug(s.login(box,password))
    except smtplib.SMTPAuthenticationError:
        s.quit()
        return False
        
    s.quit()
    return True

def send(ctx, box, token):
    s = smtplib.SMTP()
    s.connect()
    ctx.logger.debug(s.sendmail(box, box, token))
    s.quit()
    
def retr(ctx, box, password, failtag = None):
    for i in range(20):
        try:
            p = poplib.POP3('localhost')
            p.user(box)
            p.pass_(password)
            msg = p.retr(1)[1]
            p.dele(1)
            p.quit()
            return msg
        except:
            time.sleep(0.5)
            continue
    if failtag:
        ctx.fail(failtag, "failed to retrieve any messages")
    
    return None

def roundtrip(ctx, token, failtag = None):
    send(ctx, ctx.postfixcourierbox, token)
    return retr(ctx, ctx.postfixcourierbox, ctx.password, failtag)
    
def test(ctx):
    try:
        ctx.conn.rpc.classinfo(classid='Mail')
    except CoreException:
        ctx.fail("class-not-found", 'Mail class not found, skipping tests')
        return

    if postmapq('virtual_mailbox_domains', ctx.domain)[0]:
        ctx.fail("maildomain-already-in-postmap", 'Mail domain already present')
        return False
        
    mailuuid = ctx.conn.rpc.create(classid="Mail", objectid=ctx.domain, parentid=ctx.domainuuid)['body']['data']['objid']
    ctx.mailuuid = mailuuid
    ctx.logger.debug('created Mail %s (%s)' % (ctx.domain, mailuuid))

    if postmapq('virtual_mailbox_domains', ctx.domain)[0] == 'VIRTUAL\n':
        ctx.logger.debug('Mail %s spotted in postfix config' % (ctx.domain))
    else:
        ctx.fail("maildomain-not-created", 'Mail %s missing in postfix config' % (ctx.domain))
        return False
        
    box=ctx.username+'@'+ctx.domain
    ctx.postfixcourierbox=box
    alias='a_'+ctx.username+'+alias@'+ctx.domain
    
    if smtpauthcheck(ctx, box, ctx.password):
        ctx.fail("smtp-auth-false-positive", 'smtp auth works before creation of box')
        return False

    boxuuid = ctx.conn.rpc.create(classid="Mail:Box", objectid=box, parentid=mailuuid, data=dict(
        password=ctx.password,
        allowrelay=True))['body']['data']['objid']
    ctx.logger.debug('created Mail:Box %s (%s)' % (box, boxuuid))

    if not smtpauthcheck(ctx, box, ctx.password):
        ctx.fail("smtp-auth", 'smtp auth does not work')
        return False

    token = ''.join(random.choice(string.letters) for i in xrange(32)).lower()
    time.sleep(60) # horrible hack for postfix reload behaviour
    msg = roundtrip(ctx, token, "no-retr")
    if token == msg[-1]:
        ctx.logger.debug('token email sent and received correctly')
    else:
        ctx.fail("token-compare", "sent (%s) got (%s)" % (token, msg[-1]))

    aliasuuid = ctx.conn.rpc.create(classid="Mail:Alias", objectid=alias, parentid=mailuuid, data=dict(pridest='test@example.com'))['body']['data']['objid']
    ctx.conn.rpc.delete(classid="Mail:Alias", objectid=aliasuuid)

    if postmapq('virtual_alias', alias)[1]:
        ctx.fail("not-deleted", 'create+delete of alias leaves entry behind')
        return False
    
    return True

def cleanup(ctx):
    # Domain will clean it all up
    pass
