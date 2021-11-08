# RAFT Algorithm Steps (in brief)

## Leader Election

- All servers start in the follower state.
- If the followers don't receive any leader messages until a delay (*See: Election Timeout*), they become a candidate.
- The candidate requests votes from other servers.
- Servers reply with their votes, the candidate become the leader if it gets elected by the majority of servers (*See: Request Vote*).

## Log Replication

- All clients communicate only with one leader.
- Each command is added in the leader's logs but is NOT committed (still pending).
- Before committing the command, the leader sends it in the next heartbeat, to the followers.
- The leader waits until the majority of followers have written the command in their logs.
- After the leader receive an ack from the majority, it commits the command.
- The leader notifies the commit to the followers and the client.

## Heartbeat Messages

- The leader sends its *Append Entries* messages every x delay (*See: Heartbeat Timeout*)
- The term stays constant until a follower stops receiving heartbeats from the leader.
- If so, it becomes a candidate. Then a leader election occurs as a the term is incremented.

## Vocabulary

> **Election Timeout**: The amount of time a follower waits until becoming a candidate. Random delay between 150ms and 300ms.

> **Request Vote**: If the follower didn't reach the election timeout, it will vote for the candidate and reset its timeout.

> **Heartbeat Timeout**: The amount of time a leader sends its logs to the followers.

> **Append Entries**: Non committed commands.

## Documentation

- https://www.lrde.epita.fr/~renault/teaching/algorep/12-raft.pdf
- https://raft.github.io/raft.pdf
- http://thesecretlivesofdata.com/raft/
- https://zdq0394.github.io/middleware/etcd/raft/algorithm.html
