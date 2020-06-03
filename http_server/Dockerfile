# Base image
FROM ruby:2.6.6-buster AS base

WORKDIR /http_server

ADD ./Gemfile /http_server/Gemfile
ADD ./Gemfile.lock /http_server/Gemfile.lock

RUN gem install bundler -no-ri-no-rdoc && \
  bundle config set without 'development test' && \
  bundle install

# Release image
FROM base AS release

ADD . /http_server

CMD ruby http_server.rb

# Test image
FROM base AS test

RUN bundle config --delete without && \
  bundle install

CMD ruby http_server.rb
