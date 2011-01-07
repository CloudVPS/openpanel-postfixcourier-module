import smtplib, subprocess

def postmapq(mapname, key):
    return subprocess.Popen([
        'postmap',
        '-q',
        key,
        '/etc/postfix/openpanel/'+mapname
        ], stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()

def smtpauthcheck(box, password):
    s = smtplib.SMTP()
    s.connect()
    try:
        print s.login(box,password)
    except smtplib.SMTPAuthenticationError:
        s.quit()
        return False
        
    s.quit()
    return True

def test(ctx):
    try:
        ctx.conn.rpc.classinfo(classid='Mail')
    except CoreException:
        print 'Mail class not found, skipping tests'
        return

    if postmapq('virtual_mailbox_domains', ctx.domain)[0]:
        print 'Mail domain already present, ERROR'
        return False
        
    mailuuid = ctx.conn.rpc.create(classid="Mail", objectid=ctx.domain, parentid=ctx.domainuuid)['body']['data']['objid']
    print 'created Mail %s (%s)' % (ctx.domain, mailuuid)

    if postmapq('virtual_mailbox_domains', ctx.domain)[0] == 'VIRTUAL\n':
        print 'Mail %s spotted in postfix config' % (ctx.domain)
    else:
        print 'Mail %s missing in postfix config' % (ctx.domain)
        return False
        
    box=ctx.username+'@'+ctx.domain
    alias='a_'+ctx.username+'+alias@'+ctx.domain
    
    if smtpauthcheck(box, ctx.password):
        print 'smtp auth works before creation of box, ERROR'
        return False

    boxuuid = ctx.conn.rpc.create(classid="Mail:Box", objectid=box, parentid=mailuuid, data=dict(
        password=ctx.password,
        allowrelay=True))['body']['data']['objid']
    print 'created Mail:Box %s (%s)' % (box, boxuuid)

    if not smtpauthcheck(box, ctx.password):
        print 'smtp auth does not work, ERROR'
        return False

    aliasuuid = ctx.conn.rpc.create(classid="Mail:Alias", objectid=alias, parentid=mailuuid, data=dict(pridest='test@example.com'))['body']['data']['objid']
    ctx.conn.rpc.delete(classid="Mail:Alias", objectid=aliasuuid)

    if postmapq('virtual_alias', alias)[1]:
        print 'create+delete of alias leaves entry behind, ERROR'
        return False
    
    return True

def cleanup(ctx):
    pass