 <h4>Summary</h4>
  <p>You'll be developing two servers that support protocols used in e-mail.
  You can do this assignment with one partner, or you can choose to do it on
  your own.  Choose your partner carefully, we do not support "divorce", so
  once you have started the assignment with a partner you will be working on
  it with that partner until the end.  There are a number of undesirable
  consequences of pairs splitting up, so we just rule out that possibility
  in order to avoid these consquences.</p>
  <details class="card">
    <summary class="card-header">Learning Goals</summary>
    <div class="card-body">
  <ul>
    <li>To learn how to program with TCP sockets in C;</li>
    <li>To learn how to read and implement a well-specified
      protocol;</li>
    <li>To develop your programming and debugging skills as they
      relate to the use of sockets in C;</li>
    <li>To implement the server side of a protocol;</li>
    <li>To develop general networking experimental skills;</li>
    <li>To develop a further understanding of what TCP does, and how
      to manage TCP connections from the server perspective.</li>
  </ul>
  </div></details>
  <details class="card"><summary class="card-header">Special Note</summary>
  <p>Although this assignment's autograder will test the most common input
    issues and produce an 
    initial score, this score is going to be overwritten by a manual
    review by the course staff. This manual review will ensure your
    code works not only based on the simple cases listed in the
    resulting tests, but also for other considerations listed in the
    RFCs and for additional tests. TAs will also review your code
    quality in terms of clarity and use of comments.</p>
  
  <p>For consistency across the assignments, this program must be
    written in C (C++ is not acceptable). The provided code includes a
    working Makefile that will compile and link the application. Your
    code must compile with the provided file. If the program does not
    compile and link to produce an executable file, a mark of 0 (zero)
    will be awarded. Additionally, your program must compile and link
    without any warnings to receive full marks.</p>
  </details>

  <details class="card"><summary class="card-header">Your Development Environment</summary>
  <p>The assignment must compile and run on the Linux undergrad
    servers provided for student use. If you use a different
    environment be sure to check that your solution works on the
    student Linux machines, as they provide a similar environment to
    that of the grading scripts. All instructions for accessing code,
    building programs, using source code control tools, etc., assume
    you are working on these machines.</p>
  <p>If you are working in a different environment, commands and
    parameters may be different. Given the diverse collection of
    machines it is not possible for us to provide instruction and
    guidance on how to use these tools or their analogues across these
    varied environments.</p>
  <p>Pleas for leniency or special consideration of the nature "But it
    works on my laptop under ..."  will not be entertained, so make
    sure it works on the undergraduate Linux servers and allow time
    for testing in that environment so that you are confident your
    code works as expected there.</p>
  <p>The above warning is especially relevant when working with C on
    different operating systems. The precise system calls and their
    behaviour can differ significantly between different operating
    systems so just because it compiles and runs on your own machine,
    even if it is a MAC or WSL environment, do not expect it to simply
    run and work on the department Linux machines.</p>
  </details>
  <details class="card"><summary class="card-header">Assignment Overview</summary>
  <p>In this assignment you will use the Unix Socket API to construct
    two servers typically used in mail exchange: an SMTP server, used
    for sending emails, and a POP3 server, used to retrieve emails
    from a mailbox. The executables for these servers will be called,
    respectively, <code>mysmtpd</code> and <code>mypopd</code>. Both
    your programs are to take a single argument, the TCP port the
    respective server is to listen on for client connections.</p>
  <p>To start your assignment, download the
    file <a href="/pl/course_instance/2374/instance_question/21149628/clientFilesQuestion/pa_email.zip" download="">pa_email.zip</a>. This
    file contains the base files required to implement your code, as
    well as helper functions and a Makefile.</p>
  <p>In order to avoid problems with spam and misbehaved email
    messages, your servers will implement only internal email
    messages. Messages will be received by your SMTP server, saved
    locally, and then retrieved by your POP3 server. Your messages
    will not be relayed to other servers, and you will not receive
    messages from other servers. You are not allowed to implement any
    email forwarding mechanism, as unintended errors when handling
    this kind of traffic may cause the department's network to be
    flagged as an email spammer, and you may be subject to penalties
    such as account suspension or limited access to the department's
    resources.</p>
  <p>Some code is already provided that checks for the command-line
    arguments, listens for a connection, and accepts a new
    connection. Servers written in C that need to handle multiple clients
    simultaneously use the <em>fork</em> system call to handle each accepted
    client in a separate process, which means your system will naturally be
    able to handle multiple clients simultaneously without interfering with each
    other. However, debugging server processes that call <em>fork</em> can
    be challenging (as the debugger normally sees the parent process and all
    the interesting activity happens in the child).  To support this, the
    provided code disables the call to <em>fork</em> and executes all
    code in a single process unless you define the preprocessor
    symbol <code>DOFORK</code>.  We strongly suggest that you define the
    symbol <code>DOFORK</code> only when your servers are working perfectly.
  </p><p>Additionally, some provided functions also handle specific
    functionality needed for this assignment, including the ability to
    read individual lines from a socket and to handle usernames,
    passwords and email files.</p>
  </details>
  <details class="card"><summary class="card-header">Part 1: The SMTP server</summary>
  <p>The SMTP protocol, described
    in <a href="https://tools.ietf.org/html/rfc5321" target="_blank">RFC&nbsp;5321</a>, is used by mail clients (such
    as Thunderbird and Outlook) to send email messages. You will
    implement the server side of this protocol. More specifically, you
    will create a server able to support, at the very least, the
    following commands:</p>
  <ul>
    <li><code>HELO</code> and <code>EHLO</code> (client identification)</li>
    <li><code>MAIL</code> (message initialization)</li>
    <li><code>RCPT</code> (recipient specification)</li>
    <li><code>DATA</code> (message contents)</li>
    <li><code>RSET</code> (reset transmission)</li>
    <li><code>VRFY</code> (check username)</li>
    <li><code>NOOP</code> (no operation)</li>
    <li><code>QUIT</code> (session termination)</li>
  </ul>
  <p>The functionality of the commands above is listed in the RFC,
    with special consideration to sections 3 (intro), 3.1, 3.2, 3.3,
    3.5, and 3.8. Technical details about these commands are found in
    section 4 (intro), and 4.1 to 4.3. You may skip over parts of
    these sections that cover commands and functionality not listed
    above. You must pay special attention to possible error
    messages, and make sure your code sends the correct error messages
    for different kinds of invalid input, such as invalid command
    ordering, missing or extraneous parameters, or invalid
    recipients.</p>
  <p><em>About error codes</em>: The SMTP specification is sometimes not
  terribly clear about what error codes are expected in certain erroneous
  situations. In particular, section 4.3.2 indicates the various error codes
  that are possible for each command.  For example, for
  the <code>VRFY</code> command, it suggests that any of 550, 551, 553, 502,
  504 are possible. The testing framework is more
  particular than that, and expects a smaller number of error codes in the
  erroneous situations that it creates.  For failing <code>VRFY</code>
  commands, for example, the testing framework only accepts error code
  550. It will, however, let you know what error code(s) it is expecting if
  you fail a test.  Pay attention to what it says and you should be able to
  match its expectations reasonably easily.</p> 
  <p><em>About the EHLO command</em>: you are not required to
    implement the multi-line response of an EHLO command. Responding
    with a single line that contains the response code and the host
    name is enough. To retrieve the host name, you may find the result
    of the system call function <code>uname()</code> useful.</p>
  <p><em>About the VRFY command</em>: the RFC lists two possible types
    of parameters for VRFY (end of section 3.5.1): a string containing
    just the username (without <code>@domain</code>), or a string
    containing both the username and domain (a regular email address
    format, e.g. <code>john.doe@example.com</code>). You are not
    required to implement the former, only the latter. In particular,
    you only need to return success (i.e., 250) if the username exists
    and error (i.e., 550) if the username does not exist. There is no
    need to return other results listed on the RFC as <code>MAY
    implement</code> like partial matches, ambiguous results, mailbox
    storage limitations, etc..</p>
  <p>You are not required to implement commands not listed above,
    although a proper implementation must distinguish between commands
    you do not support (e.g., EXPN or HELP) from other invalid
    commands; The return code 502 is used for unsupported commands,
    while 500 is used for invalid commands. You are also not required
    to implement encryption or authentication.</p>
  <p>Although you are to accept messages from any sender, your
    recipients must be limited to the ones supported by your
    system. You are to keep in your repository directory a text file
    called <code>users.txt</code> containing, in each line, a user
    name (as an email address) and a password, separated by a
    space. You are to accept only recipients that are users in this
    file. You do not need to submit this file for grading. For
    example, your <code>users.txt</code> file may contain the
    following content:</p>
<div class="pl-code"><span><div class="mb-2 rounded" style="background: #f0f0f0"><pre style="padding: 0.5rem; margin-bottom: 0px; line-height: 125%;"><span></span>john.doe@example.com password123
mary.smith@example.com mypasswordisstrongerthanyours
edward.snowden@example.com ThisIsA100%SecureAndMemorablePassword
</pre></div>
</span></div>
  <p>Note that a single message may be delivered to more than one
    recipient, so while saving the recipients you must keep a list of
    recipients for the current message. Functionality for checking if
    a user is in the <code>users.txt</code> file and to create a list
    of users in memory is provided in the initial code of your
    repository.</p>
  <p>Once a transaction for handling a message is finished, your
    program is to save a temporary file containing the file content of
    the email message, including headers. To avoid problems with
    further code you are encouraged to save this file in your
    repository directory, not in the <code>/tmp</code> folder. You may
    use functions like <code>mkstemp</code> to create this temporary
    file. Once you create this file and store the data in it, you may
    call the function <code>save_user_mail</code>, provided in your
    initial repo code, to save the email messages for each recipient
    of the message. This function will save the message as a text file
    inside the <code>mail.store</code> directory, with subdirectories
    for each recipient. You must delete the temporary file after the
    contents of the message have been saved to the user's mailboxes.</p>
  </details>
  <details class="card"><summary class="card-header">Part 2: The POP3 server</summary>
  <p>The POP3 protocol, described
    in <a href="https://tools.ietf.org/html/rfc1939" target="_blank">RFC&nbsp;1939</a>, is used by mail clients to
    retrieve email messages from the mailbox of a specific email
    user. You will implement the server side of this
    protocol. More specifically, you are required to support, at the
    very least, the following commands:</p>
  <ul>
    <li><code>USER</code> and <code>PASS</code> (plain
      authentication)</li>
    <li><code>STAT</code> (message count information)</li>
    <li><code>LIST</code> (message listing) - with and without
      arguments</li>
    <li><code>RETR</code> (message content retrieval)</li>
    <li><code>DELE</code> (delete message)</li>
    <li><code>RSET</code> (undelete messages)</li>
    <li><code>NOOP</code> (no operation)</li>
    <li><code>QUIT</code> (session termination)</li>
  </ul>
  <p>The functionality of the commands above is listed in the
    RFC. Since this is a short RFC you are encouraged to read the
    entire document, but the most important sections are 1-6 and
    9-11. You must also pay special attention to possible error
    messages, and make sure your code sends the correct error messages
    for different kinds of invalid input, such as invalid command
    ordering, missing or extraneous parameters, or invalid
    recipients. You are not required to implement other commands. You
    are also not required to implement encryption.</p>
  <p>You only need to implement plain user authentication, with no
    password encryption or digest. User and password checks can be
    performed using provided functions, based on the information in
    file <code>users.txt</code> described above.</p>
  <p>A user's messages will be retrieved from a subdirectory
    of <code>mail.store</code>, with the subdirectory name
    corresponding to the user name. This is the same structure as the
    one used to store messages in the SMTP server; this allows you to
    use your SMTP server to add messages to a user's mailbox, and POP3
    to retrieve these messages. Your program is to read those messages
    and return them to the user as requested. Most of the
    functionality to read the list of files, obtain their size and
    store their reference in memory is already implemented in
    functions provided in file <code>mailuser.c</code>.</p>
  <p>Note that, although typical clients send messages in a
    straightforward order, you must allow the user to provide commands
    in an order that is different than a regular client, as long as
    allowed by the RFC. In particular, note that the DELE command,
    which deletes a message, does not actually delete it right away,
    but only marks it as deleted. Your code must be able to, for
    example, list all remaining messages after a deletion by ignoring
    the deleted message while still listing other messages with the
    same numeric order as they had before the deletion. The email file
    will, then, be deleted once the session terminates.</p>
  <p>Note that the autograder expects the RSET command to produce a reply
    that includes the number of messages that have been restored as the
    first thing after the +OK response.  The response line should look like:</p>
    <p></p><center>+OK <code>n</code> messages restored</center><p></p>
    <p>where <code>n</code> is the number of messages that have been restored
    (or un-deleted) as a result of the reset. <code>n</code> should be
    a non-negative number.</p>
  </details>
  
  <details class="card">
    <summary class="card-header">Implementation Constraints and Suggestions</summary>
    <div class="card-body">
  <p>The provided code already provides functionality for several
    features you are expected to perform. You are strongly encouraged
    to read the comments for the provided functions before starting
    the implementation, as part of what you are expected to implement
    can be vastly simplified by using the provided code. In
    particular, note functions like:
    </p><dl>
      <dt><code>send_formatted</code></dt>
      <dd>sends a string to the socket and allows you to use printf-like
	format directives to include extra parameters in your call</dd>
      <dt><code>nb_read_line</code></dt>
	<dd>reads from the socket and buffers the received data, while
	returning a single line at a time (similar
	  to <code>fgets</code>)</dd>
      <dt><code>dlog</code></dt>
      <dd>print a log message to the standard error stream, using
	printf-like formatting directives.  You can turn on and off logging
	by assigning the variable <code>be_verbose</code></dd>
      <dt><code>split</code></dt>
      <dd>split a line into parts that are separated by white space</dd>
      </dl>
    <p>
    You may assume that the 
    functionality of these functions will be available and unmodified
    in the grading environment.</p>
  <p>Don't try to implement this assignment all at once. Instead
    incrementally build and simultaneously test the solution. A
    suggested strategy is to:</p>
  <ul>
    <li>Start by reading the RFCs for both protocols, and listing a
      "normal" scenario, where a client will send one message to one
      recipient, or the client has one message which is listed and
      retrieved. Make note of the proper sequence of commands and
      replies to be used for each of these scenarios.</li>
    <li>Have your program send the initial welcome message and
      immediately return.</li>
    <li>Get your server to start reading and parsing commands and
      arguments. You may find library routines
      like <code>strcasecmp()</code>, <code>strncasecmp()</code>, and <code>strchr()</code>
      useful. At this point just respond with a message indicating
      that the command was not recognized.</li>
    <li>Implement simple commands like QUIT and NOOP.</li>
    <li>Implement a straightforward sequence of commands as listed in
      your first item. Start simple, then move on to more complex
      tasks.</li>
    <li>Finally, identify potentially incorrect sequences of commands
      or arguments, and handle them accordingly. Examples may include
      sending a message to no recipient, specifying a password without
      a username, obtaining the list of messages without logging in
      first, etc.</li>
  </ul></div></details>
  <details class="card">
    <summary class="card-header">Testing your code</summary>
    <div class="card-body">
      <p>For testing purposes you can use netcat (with
        the <code>-C</code> option), or use a regular email client. In
        particular, netcat can be very useful to test simple cases,
        unusual sequences of commands (e.g., retrieve message 2 before
        message 1), or incorrect behaviour. Make sure you test the
        case where multiple clients are connected simultaneously to
        your server. Also, test your solution in mixed case
        (e.g., <code>QUIT</code> vs <code>quit</code>).</p>
      <p>Do not rely on telnet and netcat for your testing only. Note
        that you must be able to use clients such as Thunderbird,
        Outlook, mutt, iOS Mail, Windows Mail, etc. to test your
        code. You may configure these clients by adding a new account
        with the following settings:</p>
      <ul>
        <li>SMTP server: the server you are
          running <code>mysmtpd</code> in
          (e.g., <code>gambier.students.cs.ubc.ca</code>), with the
          port you are using as argument;</li>
        <li>POP3 server: the server you are
          running <code>mypopd</code> in
          (e.g., <code>thetis.students.cs.ubc.ca</code>), with the
          port you are using as argument;</li>
        <li>For SMTP, specify no security/encryption and no
          authentication;</li>
        <li>For POP3, specify no security/encription and use plain
          authentication (insecure password transmission);</li>
        <li>For POP3, use an email address in <code>users.txt</code>
          as the username, with the corresponding password;</li>
        <li>Disable fetching headers only.</li>
      </ul>
      <p>If you are testing your server in a department computer, with
        a client in your own computer, you may need
        to <a href="https://it.ubc.ca/services/email-voice-internet/myvpn" target="_blank">connect to UBC's VPN</a>, since a firewall may
        block incoming connections to the department computers that
        come from outside a UBC network.</p>
      <p>An alternative mail client you can use in a text-based
        environment is mutt. If you decide to use it, you can create
        a <code>.muttrc</code> file in your home folder with the
        following contents:</p>
      <div class="pl-code"><span><div class="mb-2 rounded" style="background: #f0f0f0"><pre style="padding: 0.5rem; margin-bottom: 0px; line-height: 125%;"><span></span>set ssl_force_tls=no
set smtp_url="smtp://gambier:55555" # Replace with server/port used by mysmtpd
set pop_host="pop://thetis:44444"   # Replace with server/port used by mypopd
set pop_user="john.doe@example.com" # Must be in users.txt
set pop_pass="password123"          # Must match users.txt
set spoolfile="~/cs317_a3_inbox"
      
</pre></div>
</span></div>
      <p>You can then start the program by running <code>mutt</code>
        in your terminal. If you already use mutt for your own email,
        use a different name for the file above
        (e.g., <code>.muttrc_cs317</code>), then run the
        command <code>mutt&nbsp;-F&nbsp;.muttrc_cs317</code>.</p>
      <p>Inside the mutt interface, you can use keys
        like <code>m</code> to compose a new mail message (to test
        your SMTP server) or <code>G</code> (upper-case g) to fetch
        mail messages (to test your POP3 server). Other keys are
        listed at the top or bottom of the page, and an expanded list
        of options can be retrieved by pressing <code>?</code>.
    </p></div>
  </details>
