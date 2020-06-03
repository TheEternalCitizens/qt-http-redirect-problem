require 'sinatra'

set :bind, '0.0.0.0'

get '/login' do
  erb :login
end

post '/login' do
  redirect('overview', 302)
end

get '/overview' do
  erb :overview
end
