import smtplib, subprocess

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
    ctx.logger.debug('created Mail %s (%s)' % (ctx.domain, mailuuid))

    if postmapq('virtual_mailbox_domains', ctx.domain)[0] == 'VIRTUAL\n':
        ctx.logger.debug('Mail %s spotted in postfix config' % (ctx.domain))
    else:
        ctx.fail("maildomain-not-created", 'Mail %s missing in postfix config' % (ctx.domain))
        return False
        
    box=ctx.username+'@'+ctx.domain
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

    aliasuuid = ctx.conn.rpc.create(classid="Mail:Alias", objectid=alias, parentid=mailuuid, data=dict(pridest='test@example.com'))['body']['data']['objid']
    ctx.conn.rpc.delete(classid="Mail:Alias", objectid=aliasuuid)

    if postmapq('virtual_alias', alias)[1]:
        ctx.fail("not-deleted", 'create+delete of alias leaves entry behind')
        return False
    
    return True

def cleanup(ctx):
    pass
