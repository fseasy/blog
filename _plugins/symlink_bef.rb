#! Note: only for local dev
#  In server, `bef` will be resolved by Nginx. 
#  But in Dev, we have to link the path to make the jekyll server find the resource
require 'fileutils'

Jekyll::Hooks.register :site, :post_write do |site|
  config = site.config['local_bef_resources']
  
  # 只有配置存在且是开发环境才执行
  if config && Jekyll.env == "development"
    src_path = config['src']
    dst_name = config['dst']

    src = File.expand_path(src_path, site.source)
    dest = File.join(site.dest, dst_name)

    if File.directory?(src)
      FileUtils.rm_rf(dest) if File.exist?(dest)
      puts "  [Local Development]: Linking [#{dst_name}] resources from [#{src_path}]"
      begin
        File.symlink(src, dest)
        puts "  Succeeded."
      rescue => e
        puts "  Failed."
      end
    end
  end
end