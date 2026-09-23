from sqlalchemy import String, Integer, BigInteger, ForeignKey, UniqueConstraint, Text
from sqlalchemy.orm import DeclarativeBase, Mapped, mapped_column

class Base(DeclarativeBase): pass
class User(Base):
    __tablename__='users'
    id: Mapped[int]=mapped_column(Integer, primary_key=True)
    telegram_id: Mapped[int]=mapped_column(BigInteger, unique=True)
    name: Mapped[str]=mapped_column(String(100))
    disks: Mapped[int]=mapped_column(Integer, default=0)
class Session(Base):
    __tablename__='sessions'
    token_hash: Mapped[str]=mapped_column(String(64), primary_key=True)
    user_id: Mapped[int]=mapped_column(ForeignKey('users.id'))
    csrf_hash: Mapped[str]=mapped_column(String(64))
    expires: Mapped[int]=mapped_column(BigInteger)
class LoginProof(Base):
    __tablename__='login_proofs'
    proof: Mapped[str]=mapped_column(String(64), primary_key=True)
    expires: Mapped[int]=mapped_column(BigInteger)
class Unlock(Base):
    __tablename__='unlocks'
    user_id: Mapped[int]=mapped_column(ForeignKey('users.id'), primary_key=True)
    pack_id: Mapped[str]=mapped_column(String(80), primary_key=True)
    source: Mapped[str]=mapped_column(String(80))
class MatchResult(Base):
    __tablename__='match_results'
    id: Mapped[str]=mapped_column(String(80), primary_key=True)
    body_hash: Mapped[str]=mapped_column(String(64))
class Submission(Base):
    __tablename__='submissions'
    id: Mapped[str]=mapped_column(String(80), primary_key=True)
    user_id: Mapped[int]=mapped_column(ForeignKey('users.id'))
    puzzle_id: Mapped[str]=mapped_column(String(80))
    completed: Mapped[int]=mapped_column(Integer, default=0)
    created_at: Mapped[int]=mapped_column(BigInteger)
class OAuthState(Base):
    __tablename__='oauth_states'
    state_hash: Mapped[str]=mapped_column(String(64), primary_key=True)
    user_id: Mapped[int]=mapped_column(ForeignKey('users.id'))
    verifier: Mapped[str]=mapped_column(String(128))
    nonce: Mapped[str]=mapped_column(String(80))
    expires: Mapped[int]=mapped_column(BigInteger)
class AccountLink(Base):
    __tablename__='account_links'
    user_id: Mapped[int]=mapped_column(ForeignKey('users.id'), primary_key=True)
    issuer: Mapped[str]=mapped_column(String(300))
    subject: Mapped[str]=mapped_column(String(200))
    __table_args__=(UniqueConstraint('issuer','subject'),)
